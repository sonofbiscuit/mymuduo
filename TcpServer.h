#pragma once

/**
 * 用户使用muduo编写服务器程序
 **/

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include "Buffer.h"

// 对外的服务器编程使用的类
class TcpServer
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    enum Option   // 0和1
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop *loop,
              const InetAddress &listenAddr,
              const std::string &nameArg,
              Option option = kNoReusePort);
    ~TcpServer();

    // 这些cb都是用户自定义的
    // 可以看到testserver.cc中注册回调函数的过程
    // server_.setConnectionCallback
    // server_.setMessageCallback
    // 这些自定义的回调，将用户设置的回调传递给Tcpserver，然后TcpServer在创建新的连接的时候，传递给TcpConnection
    void setThreadInitCallback(const ThreadInitCallback &cb) { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

    // 设置底层subloop的个数
    void setThreadNum(int numThreads);  // 每个thread有一个自己的loop，因此线程数量和eventloop对应
    // 主线程在接收一个新连接的时候，会创建一个TcpConnection对象，并将这个对象对应的eventloop指向一个新的eventloop

    // 开启服务器监听
    void start();

private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;  // 当前连接的名称和对应的TcpConnection对象的指针

    EventLoop *loop_; // baseloop 用户自定义的 loop， TcpServer对应的loop

    const std::string ipPort_;  // ip端口
    const std::string name_;   // 名称

    std::unique_ptr<Acceptor> acceptor_; // 运行在mainloop 任务就是监听新连接事件

    std::shared_ptr<EventLoopThreadPool> threadPool_; // one loop per thread

    ConnectionCallback connectionCallback_;       //有新连接时的回调
    MessageCallback messageCallback_;             // 有读写事件发生时的回调
    WriteCompleteCallback writeCompleteCallback_; // 消息发送完成后的回调

    ThreadInitCallback threadInitCallback_; // loop线程初始化的回调

    std::atomic_int started_;  // 原子类型，进行原子操作   线程启动时会进行计数，防止tcpserver启动多次

    int nextConnId_;
    ConnectionMap connections_; // 保存所有的连接
};