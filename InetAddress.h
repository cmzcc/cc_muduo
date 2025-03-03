#pragma once
#include<arpa/inet.h>
#include <netinet/in.h>
#include <string>
// 封装socket地址
using namespace std;
class InetAddress
{
private:
    sockaddr_in addr_;

public:
    explicit InetAddress(uint16_t port=0, string ip = "127.0.0.1"); // 设置默认ip
    explicit InetAddress(sockaddr_in &addr)
         : addr_(addr)
     {}
     // 通过IP和端口字符串初始化InetAddress（重载构造函数）
     InetAddress(const std::string &ip, uint16_t port)
     {
         addr_.sin_family = AF_INET;
         addr_.sin_port = htons(port);                    // 网络字节序
         inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr); // 将IP字符串转换为二进制
     }
    string toIp() const;
    string toIpPort() const;
    uint16_t toPort() const;
    const struct sockaddr *getSockAddr() const
    {
        return reinterpret_cast<const struct sockaddr *>(&addr_);
    }

    void setSockAddr(const sockaddr_in &addr){addr_=addr;}
    sa_family_t family() const { return addr_.sin_family; }
};
