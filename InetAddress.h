#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

/**
 * 此函数
 * 将地址信息、ip端口这些内容封装起来
 * 并提供格式化输出
 * */
// 封装socket地址类型
class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");  // 传入 ip 和 端口
    explicit InetAddress(const sockaddr_in &addr)    // 传入sockaddr_in 结构体
        : addr_(addr)
    {
    }

    // 三种输出格式
    std::string toIp() const;
    std::string toIpPort() const;  // ip+端口，字符串形式返回
    uint16_t toPort() const;

    const sockaddr_in *getSockAddr() const { return &addr_; }
    void setSockAddr(const sockaddr_in &addr) { addr_ = addr; }

private:
    sockaddr_in addr_;
};