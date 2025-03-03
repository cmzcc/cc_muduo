
#include"EpollPoller.h"
#include"Poller.h"

#include<stdlib.h>
// 为了使基类不引用派生类的头文件，单独实现此方法
Poller* Poller::newDefaultPoller(EventLoop* loop)
{
    if(::getenv("MUDUO_USE_POLL"))
    {
        return nullptr;//如果环境中要求实现poll，则返回poll的实例
    }
    else
    {
        return new EpollPoller(loop);//否则默认返回epoll的实例
    }
}