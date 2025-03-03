#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>
#include <mutex>
// 实现单例模式，防止一个线程中创建多个EventLoop
thread_local EventLoop *t_loopInThisThread = nullptr;

// 定义默认的PollerIO复用接口的超时时间
const int kPollTimeMs = 10000;

// 创建wakeupfd,用来notify(唤醒)subReactor处理新来的channel
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d\n", errno);
    }
    return evtfd;
}
EventLoop::EventLoop()
    : looping_(false), quit_(false), callingPendingFunctors_(false), threadId_(CurrentThread::tid()), poller_(Poller::newDefaultPoller(this)), wakeupfd_(createEventfd()), wakeupChannel_(new Channel(this, wakeupfd_))

{
    LOG_DEBUG("EventLoop creadted %p in thread %d\n", this, threadId_);
    if (t_loopInThisThread)
    {
        LOG_FATAL("Anothor EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }
    // 设置wakeupfd_事件类型以及事件发生后的回调操作
    wakeupChannel_->setReadEventCallback(std::bind(&EventLoop::handleRead, this));
    // 每一个EventLoop都将监听wakeupChannel的EPOLLIN读事件
    wakeupChannel_->enableReading();
}
EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupfd_);
    t_loopInThisThread = nullptr;
}
// 开启事件循环
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while (!quit_)
    {
        activeChannels_.clear();
        // 监听两类fd,一种是client的fd,一种是wakeupfd
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (Channel *channel : activeChannels_)
        {
            // Poller监听哪些channel发生事件了，然后上报给EventLoop,通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        // 执行当前EventLoop事件循环需要处理的回调操作
        /*
        IO线程 mainloop负责监听fd, 监听到的fd,包装为channel后，交给subloop
        mainloop事先注册回调cb，需要subloop来执行          wakeup subloop之后，执行下面的方法，执行回调
        */
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
}
// 退出事件循环，1.loop在自己的线程中调用quit 2.在一个非loop对应的线程中，调用loop的quit
void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread())
    {
        wakeup();
    }
}
// 在当前loop中调用cb
void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread()) // 在当前的loop线程中，执行cb
    {
        cb();
    }
    else // 在非loop线程中执行cb,就需要先唤醒loop所在的线程，然后再执行cb
    {
        queueInLoop(cb);
    }
}
// 把cb放入队列中，唤醒loop所在的线程，执行cb
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    // 唤醒相应的，需要执行上述操作的loop线程即可
    if (!isInLoopThread() || callingPendingFunctors_) //||callingPendingFunctors_是指当前loop正在执行回调，但是loop又有了新的回调
    {
        wakeup(); // 唤醒loop所在的线程
    }
}
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupfd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8 ", n);
    }
}
// 用于唤醒loop所在的线程，向wakeupfd_中写一个数据，使wakeupChannel发生读事件，当前的loop线程就会被唤醒
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupfd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_FATAL("EventLoop::wakeup() writes %lu bytes instead of 8", n);
    }
}
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}
bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}
void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}
void EventLoop::doPendingFunctors() // 执行回调
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        // 定义一个新的局部集合，将pendingFunctors中的回调函数清空，便于新的回调继续写入，防止mainloop阻塞在这里，减少时延
        functors.swap(pendingFunctors_);
    }
    for (const Functor &functor : functors)
    {
        functor(); // 回调函数自动执行
    }
    callingPendingFunctors_ = false;
}