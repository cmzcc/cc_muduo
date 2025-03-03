#pragma once

#include <memory>
class Timer;

class TimerId
{
public:
    TimerId() : timer_(nullptr), sequence_(0) {}
    TimerId(std::shared_ptr<Timer> timer, int64_t seq)
        : timer_(timer), sequence_(seq) {}

    friend class TimerQueue;

private:
    std::shared_ptr<Timer> timer_;
    int64_t sequence_;
};
