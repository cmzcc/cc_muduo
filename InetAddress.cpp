#include"InetAddress.h"
#include<string.h>
#include<arpa/inet.h>
#include<iostream>

InetAddress::InetAddress(uint16_t port, string ip)
{

    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;                // 协议族
    addr_.sin_addr.s_addr = inet_addr(ip.c_str()); // 任意 IP 地址
    addr_.sin_port = htons(port);           // 指定端口
}
string InetAddress::toIp() const
{
    char buffer[64]={0};
    ::inet_ntop(AF_INET,&addr_.sin_addr,buffer,sizeof buffer);
    return buffer;
}
string InetAddress::toIpPort() const
{
    char buffer[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buffer, sizeof buffer);
    size_t end=strlen(buffer);
    uint16_t port=ntohs(addr_.sin_port);
    cout<<buffer<<end<<":"<<port<<endl;
    return buffer;
}
uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}
