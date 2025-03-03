#pragma once

#include<functional>
#include<vector>
#include<atomic>
#include<memory>
#include<mutex>
#include"CurrentThread.h"
#include"noncopyable.h"
#include"Timestamp.h"
// 在 EventLoop.h 中添加
#include "TimerQueue.h"
#include"Timer.h"

//事件循环类    里面包含两个大模块 Channel  Poller(epoll的抽象)

class Channel;
class Poller;
class TimerQueue;
class EventLoop:noncopyable
{
    public:
        using Functor = std::function<void()>;
        EventLoop();
        ~EventLoop();

        void loop();//开启事件循环
        void quit();//退出事件循环

        Timestamp pollReturnTime()const{return pollReturnTime_;}

        TimerId runAt(TimePoint time, Timer::TimerCallback cb);
        TimerId runAfter(Duration delay, Timer::TimerCallback cb);
        TimerId runEvery(Duration interval, Timer::TimerCallback cb);
        void cancel(TimerId timerId);
        void runInLoop(Functor cb);//在当前loop中执行cb
        void queueInLoop(Functor cb);//把cb放入队列中，唤醒loop所在的线程，执行cb

        //唤醒loop所在的线程
        void wakeup();

        //Eventloop的方法，调用EpollPoller的方法
        void updateChannel(Channel *channel);
        void removeChannel(Channel*channel);

        bool hasChannel(Channel* channel);

        //判断EventLoop对象是否在自己的线程当中
        bool isInLoopThread()const {return threadId_==CurrentThread::tid();}
        void assertInLoopThread()
        {
            if (!isInLoopThread())
            {
                abortNotInLoopThread();
            }
        }

    private:
        void abortNotInLoopThread()
        {
            throw std::runtime_error("EventLoop is not in the correct thread!");
        }

        void handleRead();//wake up
        void doPendingFunctors();//执行回调
        std::unique_ptr<TimerQueue> timerQueue_;
        using ChannelList = std::vector<Channel *>;
        std::atomic_bool looping_;//原子操作，通过CAS实现
        std::atomic_bool quit_;//标志退出loop循环

        const __pid_t threadId_;//记录当前loop所在线程的id
        Timestamp pollReturnTime_;//poller返回发生事件的channels的时间点
        std::unique_ptr<Poller>poller_;

        int wakeupfd_;//主要作用：当mainLoop获取一个新用户的channel,通过轮询算法选择一个subloop,通过该成员唤醒sublool处理channel
        std::unique_ptr<Channel>wakeupChannel_;

        ChannelList activeChannels_;

        std::atomic_bool callingPendingFunctors_; // 标识当前loop是否有需要执行的回调操作
        std::vector<Functor>pendingFunctors_;//存储loop需要执行的所有的回调操作
        std::mutex mutex_;//互斥锁，用来保护上面vector容器的线程安全操作
};

