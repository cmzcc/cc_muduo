#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include <vector>
#include <unordered_map>
class Channel; // 只调用类的指针，可以不包含头文件，而是只声明
class EventLoop;
// muduo库中多路事件分发器的核心IO复用模块
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel *>;

    Poller(EventLoop *loop);
    virtual ~Poller()=default; // 虚析构函数！普通析构会导致派生类无法调用派生类的析构，导致内存泄漏

    // 给所有的IO复用给一个接口，对于epoll类似epoll_wait
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;

    // 判断参数channel是否在当前的Poller中
    bool hasChannel(Channel *channel) const;

    // EventLoop可以通过该接口获取默认IO复用实例，类似单例模式中的getinstance
    static Poller *newDefaultPoller(EventLoop *eventloop);

protected:
    //  Map的key:sockfd value:sockfd所属的channel类型
    using ChannelMap = std::unordered_map<int, Channel *>;
    ChannelMap channels_;

private:
    EventLoop *ownerLoop_; // 定义Poller所属的事件循环EventLoop
};