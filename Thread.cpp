#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>
std::atomic_int Thread::numCreated_ (0);
Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false), joined_(false), tid_(0), func_(std::move(func)), name_(name)
{
    setDefaultName();
}
Thread::~Thread()
{
    if (started_ && !joined_)
    {
        thread_->detach(); // thread类提供的设置分离线程的方法
    }
}

void Thread::start()
{
    started_ = true;
    sem_init(&sem_, false, 0); // 初始化成员信号量
    //捕获指针，将sem_作为成员变量，防止start函数先于子线程sem_post访问sem_结束，导致sem_被销毁
    thread_ = std::shared_ptr<std::thread>(new std::thread([this]() {
        tid_ = CurrentThread::tid();
        sem_post(&sem_); // 使用成员信号量
        func_();
    }));

    sem_wait(&sem_);
    sem_destroy(&sem_); // 销毁信号量
}

void Thread::join()
{
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if (name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}