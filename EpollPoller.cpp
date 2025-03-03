#include "EpollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include<unistd.h>
#include<strings.h>
const int kNew = -1;    // channel对应的客户端在Epoll中未添加
const int kAdded = 1;   // 已添加
const int kDeleted = 2; // channel已删除

EpollPoller::EpollPoller(EventLoop *loop)
    : Poller(loop), epollfd_(epoll_create1(EPOLL_CLOEXEC)), events_(kInitEventListSize)
{
    if (epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error:%d\n", errno);
    }
}
EpollPoller::~EpollPoller()
{
    ::close(epollfd_);
}
// 重写基类poller的抽象方法
// epoll_wait
Timestamp EpollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    LOG_INFO("func=%s => fd total count:%lu\n", __FUNCTION__, channels_.size());
    int numEvents=::epoll_wait(epollfd_,&*events_.begin(),static_cast<int>(events_.size()),timeoutMs);
    int saveErrno =errno;
    Timestamp now(Timestamp::now());
    if (numEvents>0)
    {
        LOG_DEBUG("%d events happened\n",numEvents); 
        if (numEvents == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
        fillActiveChannels(numEvents,activeChannels);

        
    }
    else if (numEvents==0)
    {
        LOG_DEBUG("%s timeout!\n",__FUNCTION__);

    }
    else
    {
        if (saveErrno!=EINTR)
        {
            errno=saveErrno;//errno是全局变量
            LOG_ERROR("EpollPoller::poll() error");

        }
        
    }

    return now;
}
// Channel中调用updateChannel，实际返回Eventloop调用Poller的updateChannel,
/*
                    EventLoop
            ChannelList     Poller
                          ChannelMap<fd,Channel*>
*/
// epoll_ctl_mod
void EpollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index();
    LOG_INFO("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__,channel->fd(), channel->events(), index);

    if (index == kNew || index == kDeleted)
    {
        if (index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else // channel已经在poller上注册过
    {
        int fd = channel->fd();
        if (channel->isNoneEvent())
        {   
            channel->set_index(kDeleted);
            update(EPOLL_CTL_DEL,channel);
        }
        else
        {
            update(EPOLL_CTL_MOD,channel);
        }
        
    }
}

// epoll_ctl_del
void EpollPoller::removeChannel(Channel *channel)
{
    int fd=channel->fd();
    channels_.erase(fd);

    LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, channel->fd());
    int index = channel->index();
    if (index==kAdded)
    {
        update(EPOLL_CTL_DEL,channel);

    }
    channel->set_index(kNew);
    
}

// 填写活跃的连接
void EpollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for (int i = 0; i < numEvents; i++)
    {
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->emplace_back(channel);//c++新特性
        //EventLoop就拿到了它的poller返回给它的所有发生事件的channel列表了
    }
    
}
// 更新channel通道  epoll_ctl add mod del
void EpollPoller::update(int operation, Channel *channel)
{
    struct epoll_event event;
    bzero(&event,sizeof(event));
    event.events=channel->events();
    int fd=channel->fd();
    event.data.fd = fd;
    event.data.ptr = channel;
    if (::epoll_ctl(EpollPoller::epollfd_, operation, fd, &event)<0)
    {  
        if (operation==EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }

    
}