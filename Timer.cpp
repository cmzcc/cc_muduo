// Timer.cpp
#include "Timer.h"

std::atomic<int64_t> Timer::s_numCreated_{0};

void Timer::restart(TimePoint now)
{
    if (repeat_)
    {
        expiration_ = now + interval_;
    }
    else
    {
        expiration_ = TimePoint{};
    }
}
