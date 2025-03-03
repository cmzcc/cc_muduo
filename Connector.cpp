// Connector.cpp
// Rewritten with C++11 features

#include "Connector.h"

#include "Channel.h"
#include "EventLoop.h"
#include "SocketsOps.h"

#include <functional>
#include <chrono>
#include <random>
#include <cassert>
#include"Logger.h"
Connector::Connector(EventLoop *loop, const InetAddress &serverAddr)
    : loop_(loop),
      serverAddr_(serverAddr),
      connect_(false),
      state_(States::kDisconnected),
      retryDelayMs_(kInitRetryDelayMs)
{
}

Connector::~Connector()
{
    // Must be in loop thread, or already stopped
    assert(!channel_);
}

void Connector::start()
{
    connect_ = true;
    loop_->runInLoop([this]
                     { startInLoop(); });
}

void Connector::startInLoop()
{
    loop_->assertInLoopThread();
    assert(state_ == States::kDisconnected);
    if (connect_)
    {
        connect();
    }
}

void Connector::stop()
{
    connect_ = false;
    loop_->queueInLoop([this]
                       { stopInLoop(); });
}

void Connector::stopInLoop()
{
    loop_->assertInLoopThread();
    if (state_ == States::kConnecting)
    {
        setState(States::kDisconnected);
        int sockfd = removeAndResetChannel();
        retry(sockfd);
    }
}

void Connector::connect()
{
    int sockfd = sockets::createNonblockingOrDie(serverAddr_.family());
    int ret = sockets::connect(sockfd, serverAddr_.getSockAddr());
    int savedErrno = (ret == 0) ? 0 : errno;

    switch (savedErrno)
    {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
        connecting(sockfd);
        break;

    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
        retry(sockfd);
        break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
        sockets::close(sockfd);
        break;

    default:
        sockets::close(sockfd);
        break;
    }
}

void Connector::restart()
{
    loop_->assertInLoopThread();
    setState(States::kDisconnected);
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}

void Connector::connecting(int sockfd)
{
    setState(States::kConnecting);
    assert(!channel_);

    channel_ = std::make_unique<Channel>(loop_, sockfd);
    channel_->setWriteCallback([this]
                               { handleWrite(); });
    channel_->setErrorCallback([this]
                               { handleError(); });

    // Channel::tie() is not needed here since we hold the lifetime of Connector
    channel_->enableWriting();
}

int Connector::removeAndResetChannel()
{
    channel_->disableAll();
    channel_->remove();
    int sockfd = channel_->fd();

    // Can't reset channel_ here, because we are inside Channel::handleEvent
    loop_->queueInLoop([this]
                       { resetChannel(); });

    return sockfd;
}

void Connector::resetChannel()
{
    channel_.reset();
}

void Connector::handleWrite()
{
    if (state_ == States::kConnecting)
    {
        int sockfd = removeAndResetChannel();

        int err = sockets::getSocketError(sockfd);
        if (err)
        {
            retry(sockfd);
        }
        else if (sockets::isSelfConnect(sockfd))
        {
            retry(sockfd);
        }
        else
        {
            setState(States::kConnected);
            if (connect_)
            {
                newConnectionCallback_(sockfd);
            }
            else
            {
                sockets::close(sockfd);
            }
        }
    }
}

void Connector::handleError()
{
    if (state_ == States::kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        retry(sockfd);
    }
}

void Connector::retry(int delay)
{
    // 使用 weak_ptr 来管理 Connector 对象
    auto weak_this = std::weak_ptr<Connector>(shared_from_this());

    // 设置定时器
    loop_->runAfter(std::chrono::seconds(delay), [weak_this]()
                         {
        if (auto shared_this = weak_this.lock()) {
            // 如果对象有效，继续执行重试逻辑
            shared_this->connect();
        } else {
            // 如果对象已经销毁，避免执行
            LOG_ERROR("Connector has been destroyed, skipping retry.");
        } });
}
