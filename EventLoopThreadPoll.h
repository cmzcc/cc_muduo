#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>
#include"noncopyable.h"
class EventLoop;
class EventLoopThread;
class EventLoopThreadPoll:noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;
    EventLoopThreadPoll(EventLoop *baseloop, const std::string &nameArg);

    void setTheadNum(int numThreads) { numThreads = numThreads_; }
    void start(const ThreadInitCallback &cb = ThreadInitCallback());
    // 如果工作在多线程中,baseLoop_默认以轮询的方式分配channel给subloop
    EventLoop *getNextLoop();
    std::vector<EventLoop *> getAllLoops();

    bool started() const
    {
        return started_;
    }
    const std::string &name() const
    {
        return name_;
    }

private:
    EventLoop *baseloop_; // EventLoop loop
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread >> threads_;
    std::vector<EventLoop *> loops_;
};
