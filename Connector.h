#pragma once

#include "InetAddress.h"
#include "TimerId.h"

#include <functional>
#include <memory>
#include <atomic>

class Channel;
class EventLoop;

class Connector : public std::enable_shared_from_this<Connector>
{
public:
    using NewConnectionCallback = std::function<void(int sockfd)>;

    Connector(EventLoop *loop, const InetAddress &serverAddr);
    ~Connector();

    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        newConnectionCallback_ = cb;
    }

    void start();   // can be called in any thread
    void restart(); // must be called in loop thread
    void stop();    // can be called in any thread

    const InetAddress &serverAddress() const { return serverAddr_; }
    const int kMaxRetryDelayMs = 30 * 1000; // 30 seconds
    static const int kInitRetryDelayMs = 500;      // 0.5 seconds
private:
    enum class States
    {
        kDisconnected,
        kConnecting,
        kConnected
    };


    void setState(States s) { state_ = s; }
    void startInLoop();
    void stopInLoop();
    void connect();
    void connecting(int sockfd);
    void handleWrite();
    void handleError();
    void retry(int delayMs); // 修改参数名
    int removeAndResetChannel();
    void resetChannel();

    EventLoop *loop_;
    InetAddress serverAddr_;
    std::atomic<bool> connect_;
    std::atomic<States> state_;
    std::unique_ptr<Channel> channel_;
    NewConnectionCallback newConnectionCallback_;
    int retryDelayMs_;
    TimerId timerId_;
};

using ConnectorPtr = std::shared_ptr<Connector>;
// Connector.h
