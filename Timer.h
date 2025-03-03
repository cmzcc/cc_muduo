#pragma once

#include <atomic>
#include <functional>
#include <chrono>
#include "noncopyable.h"

using TimePoint = std::chrono::steady_clock::time_point;
using Duration = std::chrono::steady_clock::duration;

class Timer : noncopyable
{
public:
    using TimerCallback = std::function<void()>;

    Timer(TimerCallback cb, TimePoint when, Duration interval)
        : callback_(std::move(cb)),
          expiration_(when),
          interval_(interval),
          repeat_(interval.count() > 0),
          sequence_(++s_numCreated_)
    {
    }

    void run() const
    {
        if (callback_)
            callback_();
    }

    TimePoint expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }
    int64_t sequence() const { return sequence_; }
    void restart(TimePoint now);

    static int64_t numCreated() { return s_numCreated_; }

private:
    const TimerCallback callback_;
    TimePoint expiration_;
    const Duration interval_;
    const bool repeat_;
    const int64_t sequence_;

    static std::atomic<int64_t> s_numCreated_;
};
