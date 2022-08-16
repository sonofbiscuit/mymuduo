#pragma once

#include "noncopyable.h"

class InetAddress;

// 封装socket fd
class Socket : noncopyable
{
public:
    // socketfd
    // 可读：socker buffer中有数据   可写： socket buffer中还有空间让你写
    explicit Socket(int sockfd)   // 传递一个socketfd
        : sockfd_(sockfd)
    {
    }
    ~Socket();

    int fd() const { return sockfd_; }   // socket返回的socketfd
    void bindAddress(const InetAddress &localaddr);   // 绑定地址，调用了socket中的bind函数操作
    void listen();   // 监听操作
    int accept(InetAddress *peeraddr);   // 接收连接请求操作，返回一个已连接的文件描述符fd，fd代表发生的事件

    void shutdownWrite();

    void setTcpNoDelay(bool on);  // 保留糊涂窗口
    void setReuseAddr(bool on);   // 地址复用
    void setReusePort(bool on);  // 端口复用
    void setKeepAlive(bool on);  // 保活

private:
    const int sockfd_;
};
