#include "Logger.h"
#include"Timestamp.h"
#include <iostream>


/*
懒加载单例模式 实现
C++11 标准及以上规定：静态局部变量的初始化是线程安全的，
确保在多线程环境下多个线程同时调用 Logger::instance() 时，不会重复创建 Logger 对象。
返回的是 logger 的引用（Logger&），而不是值拷贝（Logger）。
这样可以避免拷贝开销，同时保证调用者使用的始终是同一个 Logger 实例
*/
// 获取日志唯一的实例对象
Logger &Logger::instance()
{
    static Logger logger;
    return logger;
}

// 设置日志级别
void Logger::setLoggerLevel(int level)
{
    LogLevel_ = level;
}

// 打印日志[级别信息] time:msg
void Logger::log(string msg)
{
    switch (LogLevel_)
    {
    case INFO:
        cout << "[INFO]";
        break;
    case ERROR:
        cout << "[ERROR]";
        break;
    case FATAL:
        cout << "[FATAL]";
        break;
    case DEBUG:
        cout << "[DEBUG]";
        break;
    default:
        break;
    }
    cout << Timestamp::now().toString() << " :   " << msg << endl;
}  