#include"EventLoopThread.h"
#include"EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name)
    : loop_(nullptr), exiting_(false), thread_(std::bind(&EventLoopThread::threadFunc, this), name), mutex_(), cond_(), callback_(cb)
{
    // 构造函数体
}

EventLoopThread::~EventLoopThread()
{
    exiting_=true;
    if (loop_!=nullptr)
    {
        loop_->quit();
        thread_.join();
    }
    
}

EventLoop* EventLoopThread::startLoop()
{
    thread_.start();//启动底层的新线程

    EventLoop *loop=nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this]
                   { return loop_ != nullptr; }); // 等待loop_初始化
        loop=loop_;
    }
    return loop;
}
//下面这个方法，是在单独的新线程中里面运行的
void EventLoopThread::threadFunc()
{
    EventLoop loop;//创建一个新线程，和上面的线程一一对应的，one loop per thread
                   // 栈对象，线程结束时自动销毁,无需delete
    if (callback_)
    {
        callback_(&loop);

    }
    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_=&loop;
        cond_.notify_one(); // 通知主线程
    }
    loop.loop();//开启Poller
    std::unique_lock<std::mutex> lock(mutex_);
    loop_=nullptr;
}
// 在 EventLoop.cpp 中添加实现
TimerId EventLoop::runAt(TimePoint time, Timer::TimerCallback cb)
{
    return timerQueue_->addTimer(std::move(cb), time, Duration(0));
}

TimerId EventLoop::runAfter(Duration delay, Timer::TimerCallback cb)
{
    return runAt(std::chrono::steady_clock::now() + delay, std::move(cb));
}

TimerId EventLoop::runEvery(Duration interval, Timer::TimerCallback cb)
{
    return timerQueue_->addTimer(std::move(cb),
                                 std::chrono::steady_clock::now() + interval, interval);
}

void EventLoop::cancel(TimerId timerId)
{
    timerQueue_->cancel(timerId);
}