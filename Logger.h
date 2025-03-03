#pragma once
#include<string>
#include<stdlib.h>
#include"noncopyable.h"

using namespace std;
//定义日志的级别 INFO   ERROR   FATAL   DEBUG
enum LogLevel
{
    INFO,//普通信息
    ERROR,//错误信息
    FATAL,//core信息（严重错误）
    DEBUG,//调试信息
};
// LOG_INFO(%s %d ,arg1,arg2)
#define LOG_INFO(logmsgFormat, ...)                       \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLoggerLevel(INFO);                      \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)
#define LOG_ERROR(logmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLoggerLevel(ERROR);                     \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)

#define LOG_FATAL(logmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLoggerLevel(FATAL);                     \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
        exit(-1);                                 \
    } while (0)
#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLoggerLevel(DEBUG);                     \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)
#else
#define LOG_DEBUG(logmsgFormat, ...)
#endif
// 输出一个日志类
class Logger:noncopyable
{
public:
    //获取日志的唯一实例对象
    static Logger& instance();
    //设置日志级别
    void setLoggerLevel(int level);
    //打印日志
    void log(string msg);
private:
    int LogLevel_;
    Logger(){}
};