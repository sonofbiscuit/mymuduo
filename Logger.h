//
// Created by 张庆 on 2022/7/25.
//

#pragma once


#include "nonCopyable.h"
#include <iostream>
#include <string>


/** 定义宏 LOG_INFO
 * 可变参
 * snprintf(char* str, size_t size, const char *format, ...)
 * 将可变参数 ... 按照format的格式 格式化为字符串，然后将其拷贝进str中，类似缓冲区
 * ##__VA_ARGS__ 就是复制下来括号里 ... 的值
 */
#define LOG_INFO(logmsgFormat, ...)                       \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLogLevel(INFO);                         \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)

#define LOG_ERROR(logmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLogLevel(ERROR);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)

#define LOG_FATAL(logmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLogLevel(FATAL);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
        exit(-1);                                         \
    } while (0)

#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLogLevel(DEBUG);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)
#else
#define LOG_DEBUG(logmsgFormat, ...)
#endif


enum logLevel {
    INFO,  // 普通msg
    ERROR,  // 错误信息
    FATAL,  // core dump
    DEBUG,  // debug info
};

class Logger : nonCopyable {
public:
    // 获取当前日志唯一的实例对象， 单例模式(当前类在当前进程只能有一个实例
    // static 成员函数。该函数不接受this指针，只能访问类的静态成员，不能直接访问普通成员(通过对象访问)。是整个类共享的
    static Logger &instance();

    // 设置日志级别
    void setLogLevel(int level);

    // 写日志
    void log(std::string msg);

private:
    int logLevel_;
};


