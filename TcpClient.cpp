// TcpClient.cpp
// Rewritten with C++11 features

#include "TcpClient.h"

#include "Connector.h"
#include "EventLoop.h"
#include "SocketsOps.h"

#include <stdio.h> // snprintf
#include <cassert>
#include "Logger.h"
#include <strings.h>


void removeConnection(EventLoop *loop, const TcpConnectionPtr &conn)
{
    loop->queueInLoop([conn]
                      { conn->connectDestroyed(); });
}

TcpClient::TcpClient(EventLoop *loop,
                     const InetAddress &serverAddr,
                     const std::string &nameArg)
    : loop_(loop),
      connector_(std::make_shared<Connector>(loop, serverAddr)),
      name_(nameArg),
      connectionCallback_(),
      messageCallback_(),
      writeCompleteCallback_(),
      retry_(false),
      connect_(true),
      nextConnId_(1)
{
    connector_->setNewConnectionCallback(
        [this](int sockfd)
        { newConnection(sockfd); });
    // FIXME setConnectFailedCallback
}

TcpClient::~TcpClient()
{
    TcpConnectionPtr conn;
    bool unique = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        unique = connection_.unique();
        conn = connection_;
    }

    if (conn)
    {
        assert(loop_ == conn->getLoop());
        // FIXME: not 100% safe, if we are in different thread
        CloseCallback cb = [this](const TcpConnectionPtr &conn)
        {
            removeConnection(conn);
        };
        loop_->runInLoop([&conn, cb]()
                         {
            conn->setCloseCallback(cb);
            conn->shutdown(); });

    }
    else
    {
        connector_->stop();

    }
}

void TcpClient::connect()
{
    connect_ = true;
    connector_->start();
}

void TcpClient::disconnect()
{
    connect_ = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (connection_)
        {
            connection_->shutdown();
        }
    }
}

void TcpClient::stop()
{
    connect_ = false;
    connector_->stop();
}

void TcpClient::newConnection(int sockfd)
{
    loop_->assertInLoopThread();
    // 通过sockfd获取其绑定的本机的ip地址和端口信息
    sockaddr_in peer;
    ::bzero(&peer, sizeof peer);
    socklen_t addrlen = sizeof peer;
    if (::getsockname(sockfd, (sockaddr *)&peer, &addrlen) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress peerAddr(peer);
   
    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    // 通过sockfd获取其绑定的本机的ip地址和端口信息
    sockaddr_in local;
    ::bzero(&local, sizeof local);
    addrlen = sizeof local;
    if (::getsockname(sockfd, (sockaddr *)&local, &addrlen) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddr(local);

    TcpConnectionPtr conn = std::make_shared<TcpConnection>(
        loop_,
        connName,
        sockfd,
        localAddr,
        peerAddr);

    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback([this](const TcpConnectionPtr &conn)
                           { removeConnection(conn); });

    {
        std::lock_guard<std::mutex> lock(mutex_);
        connection_ = conn;
    }

    conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->assertInLoopThread();
    assert(loop_ == conn->getLoop());

    {
        std::lock_guard<std::mutex> lock(mutex_);
        assert(connection_ == conn);
        connection_.reset();
    }

    loop_->queueInLoop([conn]
                      {
                          if (conn) {
                              conn->connectDestroyed();
                          } });

    if (retry_ && connect_)
    {
        connector_->restart();
    }
}
