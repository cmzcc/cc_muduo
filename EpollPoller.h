#pragma once

#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

class Channel;
/*
epoll的使用
epoll_create
epoll_ctl   add/mod/del
epoll_wait
*/
class EpollPoller :public Poller
{
public:
    EpollPoller(EventLoop *loop);
    ~EpollPoller() override;
    //重写基类poller的抽象方法
    
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;//epoll_wait
    void updateChannel(Channel *channel) override;//epoll_ctl_mod
    void removeChannel(Channel *channel) override;//epoll_ctl_del

private:
static const int kInitEventListSize=16;
    //填写活跃的连接
    void fillActiveChannels(int numEvents,ChannelList *activeChannels)const;
    //更新channel通道
    void update(int operation,Channel *channel);

    using EventList = std::vector<epoll_event>;//evs

    int epollfd_;
    EventList events_;
};
