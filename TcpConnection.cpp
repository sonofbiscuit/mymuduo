#include <functional>
#include <string>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>

#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

static EventLoop *CheckLoopNotNull(EventLoop *loop)  // /判断一个eventLoop指向是否为空
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null!\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

// void TcpServer::newConnection会调用
TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &nameArg,
                             int sockfd,  // 接收连接以后返回的connectionfd
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(CheckLoopNotNull(loop))
    , name_(nameArg)
    , state_(kConnecting)  //四种状态中的kConnecting
    , reading_(true)
    , socket_(new Socket(sockfd))    //
    , channel_(new Channel(loop, sockfd))
    , localAddr_(localAddr)
    , peerAddr_(peerAddr)
    , highWaterMark_(64 * 1024 * 1024) // 64M
{
    // 下面给channel设置相应的回调函数 poller给channel通知感兴趣的事件发生了 channel会回调相应的回调函数
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this));

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d\n", name_.c_str(), channel_->fd(), (int)state_);
}

void TcpConnection::send(const std::string &buf)
{
    if (state_ == kConnected)  // 已连接状态
    {
        // 当前所属线程调用
        if (loop_->isInLoopThread()) // 这种是对于单个reactor的情况 用户调用conn->send时 loop_即为当前线程
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else  // 外部线程调用
        {
            loop_->runInLoop(
                std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));   // 加到pendingFunctors_队列里
        }
    }
}

/**
 * 发送数据 应用写的快 而内核发送数据慢 需要把待发送数据写入缓冲区，而且设置了水位回调
 *
 * 发送数据：fd->outputBuffer->内核缓冲区
 * 当内核缓冲区满了，那么我们需要降低之前的发送速率
 **/
void TcpConnection::sendInLoop(const void *data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    if (state_ == kDisconnected) // 之前调用过该connection的shutdown 不能再进行发送了
    {
        LOG_ERROR("disconnected, give up writing");
    }

    // 表示channel_第一次开始写数据或者缓冲区没有待发送数据
    // 判断channel是否关注了写事件
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)  // outputBuffer_.readableBytes()为0说明内核缓冲区没满
    {
        nwrote = ::write(channel_->fd(), data, len);   // channel对应的fd中进行写操作，直接写到内核缓冲区
        if (nwrote >= 0)
        {
            remaining = len - nwrote;   // 剩下的数据长度
            if (remaining == 0 && writeCompleteCallback_)
            {
                // 既然在这里数据全部发送完成，就不用再给channel设置epollout事件了
                loop_->queueInLoop(
                    std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else // nwrote < 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK) // EWOULDBLOCK表示非阻塞情况下没有数据后的正常返回 等同于EAGAIN
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                if (errno == EPIPE || errno == ECONNRESET) // SIGPIPE RESET
                {
                    faultError = true;
                }
            }
        }
    }
    /**
     * 说明当前这一次write并没有把数据全部发送出去 剩余的数据需要保存到缓冲区当中
     * 然后给channel注册EPOLLOUT事件，Poller发现tcp的发送缓冲区有空间后会通知
     * 相应的sock->channel，调用channel对应注册的writeCallback_回调方法，
     * channel的writeCallback_实际上就是TcpConnection设置的handleWrite回调，
     * 把发送缓冲区outputBuffer_的内容全部发送完成
     **/
    if (!faultError && remaining > 0)   // 没写完
    {
        // 目前发送缓冲区剩余的待发送的数据的长度
        size_t oldLen = outputBuffer_.readableBytes();   // 先把outputBuffer中可读的长度记录下来
        // 待读的长度加上没写完的长度  大于  高水位线
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_)
        {
            loop_->queueInLoop(
                std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));  // 执行高水位回调
        }
        outputBuffer_.append((char *)data + nwrote, remaining);    // 将没写完的内容，写入到outputbuffer之中
        if (!channel_->isWriting())
        {
            // 让channel关注写事件，因为一旦内核缓冲区有空间，就会触发写事件
            channel_->enableWriting(); // 这里一定要注册channel的写事件 否则poller不会给channel通知epollout
        }
    }
}

void TcpConnection::shutdown()
{
    if (state_ == kConnected)   // 已连接
    {
        setState(kDisconnecting);  // 变为正在关闭
        loop_->runInLoop(
            std::bind(&TcpConnection::shutdownInLoop, this));  // 调用shutdownInLoop   半关闭
    }
}

void TcpConnection::shutdownInLoop()
{
    //
    if (!channel_->isWriting()) // 说明当前outputBuffer_的数据全部向外发送完成
    {
        socket_->shutdownWrite();
    }
}

// 连接建立
void TcpConnection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading(); // 向poller注册channel的EPOLLIN读事件

    // 新连接建立 执行回调
    connectionCallback_(shared_from_this());   // acceptor设置的NewConnectionCallback_
    // 调用了用户传入的connectionCallback_
    // cb的传入过程   用户设置->tcpserver->tcpconnection->tcpconnection再调用
}
// 连接销毁
void TcpConnection::connectDestroyed()
{
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll(); // 把channel的所有感兴趣的事件从poller中删除掉
        connectionCallback_(shared_from_this());
    }
    channel_->remove(); // 把channel从poller中删除掉
}

/**
 * 缓冲区存入数据，由空变为不空，那么就变成了可读状态
 * 缓冲区由满变为不满，就变成了可写
 * */

// 读是相对服务器而言的
// 当对端客户端有数据到达 服务器端检测到EPOLLIN 就会触发该fd上的回调 handleRead取读走对端发来的数据
void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    // 已连接的文件描述符对应的数据读入到内核缓冲区
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0) // 有数据到达
    {
        // 已建立连接的用户有可读事件发生了 调用用户传入的回调操作onMessage shared_from_this就是获取了TcpConnection的智能指针
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0) // 客户端断开
    {
        handleClose();
    }
    else // 出错了
    {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if (channel_->isWriting())
    {
        int savedErrno = 0;
        // 写n个数据
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);  // 将outputBuffer中readable部分的内容写入到内核缓冲区中
        if (n > 0)
        {
            // 判断数据是否完全写完
            outputBuffer_.retrieve(n);   // 取n个字节，因为已经把自定义缓冲区里的数据写了n个到内核缓冲区，因此把这部分数据取出来
            // 如果没有存完，那么n>0，这个if里不执行。等到内核缓冲区由满变为不满，
            // 当内核缓冲区空出来后，又会触发写事件，那么会再次执行handleWrite，直至数据全部写进去
            if (outputBuffer_.readableBytes() == 0)
            {
                // 当数据全都写进去之后，要关闭写事件
                // 因为内核缓冲区一旦有空间，就会触发写事件，
                // 不关闭的话，epoll就会一直循环不会阻塞，触发写事件
                // 读事件不需要关闭，因为是监测内核缓冲区是否有数据到来，有数据才需要读事件
                channel_->disableWriting();
                if (writeCompleteCallback_)   // 如果设置了，那么就把设置的cb加到队列当中。没设置则不管
                {
                    // TcpConnection对象在其所在的subloop中 向pendingFunctors_中加入回调
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting)   // 关闭写，但可读，半关闭
                {
                    shutdownInLoop(); // 在当前所属的loop中把TcpConnection删除掉
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else
    {
        LOG_ERROR("TcpConnection fd=%d is down, no more writing", channel_->fd());
    }
}

void TcpConnection::handleClose()
{
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d\n", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr); // 执行连接关闭的回调
    // TcpServer::removeConnection
    closeCallback_(connPtr);      // 执行关闭连接的回调 执行的是TcpServer::removeConnection回调方法   // must be the last line
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d\n", name_.c_str(), err);
}