#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"

static int createNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d listen socket create err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)  //上层传入的loop
    : loop_(loop)
    , acceptSocket_(createNonblocking())  // 将一个socket创建以后形成的fd，调用socket的构造函数，赋值给socket::sockfd_
    , acceptChannel_(loop, acceptSocket_.fd())  // channel类和文件描述符绑定在一起，同时给eventloop指针，对应主reactor的loop指针
    // acceptChannel_和socketfd进行绑定
    // 绑定给主reactor的原因是主reactor负责处理请求
    , listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr);
    // TcpServer::start() => Acceptor.listen() 如果有新用户连接 要执行一个回调(accept => connfd => 打包成Channel => 唤醒subloop)
    // baseloop监听到有事件发生 => acceptChannel_(listenfd) => 执行该回调函数
    acceptChannel_.setReadCallback(   // 绑定读回调
        std::bind(&Acceptor::handleRead, this));  // 当有连接请求的时候，就会调用handleRead
    /**
     * 对于channel绑定的回调函数，eventloop检测到后，会触发相应函数
     * for (Channel *channel : activeChannels_)
        {
            // Poller监听哪些channel发生了事件 然后上报给EventLoop 通知channel处理相应的事件
            channel->handleEvent(pollRetureTime_);
        }
     * */

}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();    // 把从Poller中感兴趣的事件删除掉   update()取消
    // 在channel所属的EventLoop中把当前的channel删除掉
    acceptChannel_.remove();        // 调用EventLoop->removeChannel => Poller->removeChannel 把Poller的ChannelMap对应的部分删除
}

// 监听调用的是socket的listen
// 监听完以后，再channel上绑定一个enableReading，使fd可读
// 只有监听以后，才会进行读事件，否则不会
void Acceptor::listen()
{
    listenning_ = true;
    acceptSocket_.listen();         // listen
    acceptChannel_.enableReading(); // acceptChannel_注册至Poller !重要
}

// 响应连接请求
// listenfd有事件发生了，就是有新用户连接了
// handleRead ->
void Acceptor::handleRead()
{
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);  // socket接收，生成connectfd。实现为调用linux下的accept4，生成connectfd
    if (connfd >= 0)  // 成功
    {
        if (NewConnectionCallback_)   // tcpserver的构造函数中设置了NewConnectionCallback_
        {
            /**
             * 执行相应的回调函数，tcpserver构造函数时候，就已经给acceptor绑定了需要的回调函数
             * acceptor_->setNewConnectionCallback(
             * std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
            */
            // void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
            NewConnectionCallback_(connfd, peerAddr); // 轮询找到subLoop 唤醒并分发当前的新客户端的Channel
        }
        else
        {
            ::close(connfd);
        }
    }
    else
    {
        // 陈硕的实现，是用一个dev/null空闲描述符
        // 遇到fd满的时候，先关闭这个空闲描述符，去accept(2)现在传来的描述符
        // 之后再关闭现在的连接  close(2)
        // 然后再打开占位的空闲描述符
        LOG_ERROR("%s:%s:%d accept err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        if (errno == EMFILE)   // EMFILE: too many open files 即文件描述符已经达到最大上限，ulimia -a可以查看上限
        {
            LOG_ERROR("%s:%s:%d sockfd reached limit\n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}