#pragma once

#include <functional>

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

class EventLoop;
class InetAddress;


// 对于多reactor模型来说，accrptor对应的就是主reactor
class Acceptor : noncopyable
{
public:
    // 当连接到达时 就会使用这个回调，也叫做上层回调
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &)>;  // 回调函数

    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
    ~Acceptor();

    // 设置回调
    void setNewConnectionCallback(const NewConnectionCallback &cb) { NewConnectionCallback_ = cb; }

    bool listenning() const { return listenning_; }
    void listen();

private:
    void handleRead();  // 是个回调函数

    EventLoop *loop_; // Acceptor用的就是用户定义的那个baseLoop 也称作mainLoop， 永远指向tcpserver传给我们的主loop
    Socket acceptSocket_;   // socket类
    Channel acceptChannel_;   // 一个文件描述符必定伴随一个channel
    NewConnectionCallback NewConnectionCallback_;  // 回调函数
    bool listenning_;
};