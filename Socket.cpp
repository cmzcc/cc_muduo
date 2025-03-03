#include"Socket.h"  
#include"Logger.h"
#include"InetAddress.h"
#include<strings.h>
#include<unistd.h>
#include <netinet/tcp.h> // 包含 TCP_NODELAY 的定义

#include<sys/types.h>
#include<sys/socket.h>

Socket::~Socket()
{
    close(socketfd_);
}



void Socket::bindAddress(const InetAddress &localaddr)
{
    if(0!=bind(socketfd_,(sockaddr*)localaddr.getSockAddr(),sizeof(sockaddr_in)))
    {
        LOG_FATAL("bind sockfd:%d fail \n",socketfd_);
    }
}
void Socket::listen()
{
    if (0!=::listen(socketfd_,1024))
    {
        LOG_FATAL("listen sockfd:%d fail \n",socketfd_);
    }
    
}
int Socket::accept(InetAddress *peeraddr)
{
    sockaddr_in addr;
    socklen_t len=sizeof addr;
    bzero(&addr,sizeof addr);
    int connfd=::accept4(socketfd_,(sockaddr*)&addr,&len,SOCK_NONBLOCK|SOCK_CLOEXEC);
    if (connfd>0)
    {
        peeraddr->setSockAddr(addr);
    }
    return connfd;

}

void Socket::shutdownWrite()
{
    if (::shutdown(socketfd_,SHUT_WR)<0)
    {
        LOG_ERROR("shutdownWrite error");
    }
    
}

void Socket::setTcpNoDelay(bool on)
{
    int optval=on?1:0;
    ::setsockopt(socketfd_,IPPROTO_TCP,TCP_NODELAY,&optval,sizeof optval);
}
void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(socketfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
}
void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(socketfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval);
}
void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(socketfd_, IPPROTO_TCP, SO_KEEPALIVE, &optval, sizeof optval);
}