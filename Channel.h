#pragma once
#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>
class EventLoop;
class Timestamp;

/*
    EventLoop,Channel,Poller之间的关系      在Reactor模型上对应Demultiplex
    Channel理解为通道，封装了sockfd和其感兴趣的eventfd，如EPOLLIN,EPOLLOUT事件
    还绑定了poller返回的具体事件
*/

class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);

    ~Channel();
    // fd得到poller通知以后，处理事件的
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象

    void setReadEventCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 防止当channel被手动remove掉，channel还在执行回调操作
    void tie(const std::shared_ptr<void> &);

    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revents(int revt) { revents_ = revt; }

    bool isNoneEvent() const { return events_ == kNoneEvent; }

    // 设置fd相应的事件状态
    void enableReading()
    {
        events_ |= kReadEvent;
        update();
    }
    void disableReading()
    {
        events_ &= ~kReadEvent;
        update();
    }
    void enableWriting()
    {
        events_ |= kWriteEvent;
        update();
    }
    void disableWriting()
    {
        events_ &= ~kWriteEvent;
        update();
    }
    void disableAll()
    {
        events_ = kNoneEvent;
        update();
    }
    // 返回fd当前的事件状态
    bool isNondeEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    // one loop per thread
    EventLoop *ownerLoop() { return loop_; }
    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_; // 事件循环
    const int fd_;    // fd,Poller监听的对象
    int events_;      // 注册fd感兴趣的事件（通过位运算组合）
    int revents_;     // poller返回的具体事件
    int index_;       //在 Poller 中的状态标识（ - 1 = 未注册，1 = 已注册，2 = 已删除）

     std::weak_ptr<void>tie_;             // 绑定的对象（如 TcpConnection）的弱引用，用于防止回调时对象已被销毁
    bool tied_;               //	标识是否已绑定对象

    // 因为channel通道里面能够获知fd最终发生的事件events，所以它负责调用具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback errorCallback_;
    EventCallback closeCallback_;
};
