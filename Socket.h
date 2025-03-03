#pragma once
#include"noncopyable.h"
class InetAddress;

//封装socket fd
class Socket : noncopyable
{
public:
    explicit Socket(int socketfd)
    :socketfd_(socketfd)
    {

    }
    ~Socket();
    int fd()const{return socketfd_;}
    void bindAddress(const InetAddress &localaddr);
    void listen();
    int accept(InetAddress *peeraddr);

    void shutdownWrite();

    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);
private:
    const int socketfd_;
};