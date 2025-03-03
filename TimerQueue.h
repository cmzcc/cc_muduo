#pragma once
#include <set>
#include <vector>
#include <memory>
#include "Channel.h"
#include "TimerId.h"
#include "noncopyable.h"
#include"Timer.h"
class EventLoop;
class Timer;

class TimerQueue : noncopyable
{
public:
    explicit TimerQueue(EventLoop *loop);
    ~TimerQueue();

    TimerId addTimer(Timer::TimerCallback cb,
                     TimePoint when,
                     Duration interval);

    void cancel(TimerId timerId);

private:
    using Entry = std::pair<TimePoint, std::shared_ptr<Timer>>;
    using TimerList = std::set<Entry>;

    void handleRead();
    std::vector<Entry> getExpired(TimePoint now);
    void reset(const std::vector<Entry> &expired, TimePoint now);
    bool insert(std::shared_ptr<Timer> timer);

    EventLoop *loop_;
    const int timerfd_;
    Channel timerfdChannel_;
    TimerList timers_;
};
