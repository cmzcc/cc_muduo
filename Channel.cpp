#include"Channel.h"
#include"EventLoop.h"
#include"Logger.h"

#include<sys/epoll.h>
/*
epoll事件	值（二进制）	    说明
EPOLLIN	    0b0001	    数据可读（包括 TCP 带外数据）
EPOLLPRI	0b0010	    紧急数据可读（高优先级数据）
EPOLLOUT	0b0100	    数据可写
EPOLLHUP	0b1000	    连接挂断（对端关闭）
EPOLLERR	0b10000	    错误发生（如 Socket 错误）
*/
const int Channel::kNoneEvent=0;
const int Channel:: kReadEvent=EPOLLIN|EPOLLPRI;
const int Channel::kWriteEvent=EPOLLOUT;

//EventLoop: ChannelList Poller
Channel::Channel(EventLoop *loop, int fd)
        :  loop_(loop)
        ,fd_(fd)
        ,events_(0)
        ,revents_(0)
        ,index_(-1)
        ,tied_(false)
{

}
Channel::~Channel()
{

}
//TcpConnection建立时绑定channel,channel用一个弱智能指针指向TcpConnenction,防止调用TcpConnection中的回调时，已被销毁，导致未定义行为
void Channel::tie(const std::shared_ptr<void>&obj)   
{
    tie_=obj;
    tied_=true;
}
/*
    当改变channel所表示fd的events事件后，update负责在poller里面更改fd相应的事件epoll_ctl
    Channel通过调用所属Eventloop中的方法，更改poller中的状态
*/
void Channel::update()
{
    //通过channel所属的Eventloop,调用poller的相应方法，注册fd的events事件
    loop_->updateChannel(this);
}
void Channel::remove()
{
    //同样调用Eventloop中的方法删除
    loop_->removeChannel(this);
}
void Channel::handleEvent(Timestamp receiveTime)
{
    if(tied_)
    {
        std::shared_ptr<void> guard =tie_.lock();//将弱智能指针提升为强智能指针
        if(guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    
    }
}
//根据poller通知的channel发生的具体实践，由channel负责调用具体的回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
     LOG_INFO("channel handleEvent revents:%d\n",revents_);

    // 优先处理错误事件
    if (revents_ & EPOLLERR)
    {
        if (errorCallback_)
        {
            errorCallback_();
        }
        else
        {
            LOG_ERROR("No error callback set for fd:%d", fd_);
        }
        return; // 错误发生后终止处理
    }

    // 处理连接关闭事件（含半关闭）
    if (revents_ & (EPOLLHUP | EPOLLRDHUP))
    {
        if (closeCallback_)
        {
            closeCallback_();
        }
        else
        {
            LOG_ERROR("No close callback set for fd:%d", fd_);
        }
        return; // 连接关闭后终止处理
    }

    // 处理可读事件（含普通数据和紧急数据）
    if (revents_ & (EPOLLIN | EPOLLPRI))
    {
        if (readCallback_)
        {
            readCallback_(receiveTime);
        }
        else
        {
            LOG_FATAL("Critical: No read callback for fd:%d", fd_);
        }
    }

    // 处理可写事件
    if (revents_ & EPOLLOUT)
    {
        if (writeCallback_)
        {
            writeCallback_();
        }
        else
        {
            LOG_DEBUG("Spurious write event on fd:%d", fd_);  
        }
    }
}