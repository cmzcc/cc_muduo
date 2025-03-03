// TimerQueue.cpp
#include "TimerQueue.h"
#include "EventLoop.h"
#include "Timer.h"
#include "Logger.h"
#include <sys/timerfd.h>
#include <unistd.h>
#include <string.h>



namespace detail
{
    int createTimerfd()
    {
        int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
        if (timerfd < 0)
        {
            LOG_FATAL("Failed in timerfd_create");
        }
        return timerfd;
    }

    void readTimerfd(int timerfd, TimePoint now)
    {
        uint64_t howmany;
        ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
        if (n != sizeof howmany)
        {
            LOG_ERROR("TimerQueue::handleRead() reads %ld bytes instead of 8", n);
        }
    }

    struct timespec howMuchTimeFromNow(TimePoint when)
    {
        auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(
                                when - std::chrono::steady_clock::now())
                                .count();

        if (microseconds < 100)
        {
            microseconds = 100;
        }

        struct timespec ts;
        ts.tv_sec = static_cast<time_t>(microseconds / 1000000);
        ts.tv_nsec = static_cast<long>((microseconds % 1000000) * 1000);
        return ts;
    }

    void resetTimerfd(int timerfd, TimePoint expiration)
    {
        struct itimerspec newValue;
        struct itimerspec oldValue;
        memset(&newValue, 0, sizeof newValue);
        memset(&oldValue, 0, sizeof oldValue);

        newValue.it_value = howMuchTimeFromNow(expiration);
        int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
        if (ret)
        {
            LOG_ERROR("timerfd_settime()");
        }
    }

        } // namespace detail

        TimerQueue::TimerQueue(EventLoop *loop)
            : loop_(loop),
              timerfd_(detail::createTimerfd()),
              timerfdChannel_(loop, timerfd_)
        {
            timerfdChannel_.setReadEventCallback(std::bind(&TimerQueue::handleRead, this));
            timerfdChannel_.enableReading();
        }

        TimerQueue::~TimerQueue()
        {
            timerfdChannel_.disableAll();
            ::close(timerfd_);
        }

        TimerId TimerQueue::addTimer(Timer::TimerCallback cb, TimePoint when, Duration interval)
        {
            auto timer = std::make_shared<Timer>(std::move(cb), when, interval);

            // 确保 loop_ 指针有效
            if (!loop_)
            {
                LOG_ERROR("TimerQueue::addTimer - EventLoop is null");
                return TimerId();
            }

            // 使用 queueInLoop 代替 runInLoop，避免复杂的闭包生命周期管理
            loop_->queueInLoop([this, timer = std::move(timer)]() mutable
                               {
        if (this && loop_) {  // 额外的安全检查
            insert(std::move(timer));
        } });

            return TimerId(timer, timer->sequence());
        }

        void TimerQueue::cancel(TimerId timerId)
        {
            loop_->runInLoop([this, timerId]
                             {
        auto it = timers_.begin();
        for (; it != timers_.end(); ++it) {
            if (it->second->sequence() == timerId.sequence_) {
                timers_.erase(it);
                break;
            }
        } });
        }

        void TimerQueue::handleRead()
        {
            TimePoint now = std::chrono::steady_clock::now();
            detail::readTimerfd(timerfd_, now);

            std::vector<Entry> expired = getExpired(now);
            for (const auto &entry : expired)
            {
                entry.second->run();
            }

            reset(expired, now);
        }

        std::vector<TimerQueue::Entry> TimerQueue::getExpired(TimePoint now)
        {
            std::vector<Entry> expired;
            Entry sentry = std::make_pair(now, nullptr);
            auto end = timers_.lower_bound(sentry);
            std::copy(timers_.begin(), end, back_inserter(expired));
            timers_.erase(timers_.begin(), end);
            return expired;
        }

        void TimerQueue::reset(const std::vector<Entry> &expired, TimePoint now)
        {
            for (const auto &entry : expired)
            {
                if (entry.second->repeat())
                {
                    entry.second->restart(now);
                    insert(entry.second);
                }
            }

            if (!timers_.empty())
            {
                auto nextExpire = timers_.begin()->first;
                detail::resetTimerfd(timerfd_, nextExpire);
            }
        }

        bool TimerQueue::insert(std::shared_ptr<Timer> timer)
        {
            bool earliestChanged = false;
            TimePoint when = timer->expiration();
            auto it = timers_.begin();
            if (it == timers_.end() || when < it->first)
            {
                earliestChanged = true;
            }

            auto result = timers_.insert(Entry(when, timer));
            return earliestChanged;
        }

