//
// Created by 张庆 on 2022/7/25.
//

#include <iostream>

#include "Logger.h"
#include "TimeStamp.h"

// 单例对象
Logger &Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::setLogLevel(int level) {
    logLevel_ = level;
}

/**
 * 写日志
 * [loglevel] : time : msg
 * */
void Logger::log(std::string msg) {
    switch (logLevel_) {
        case INFO:
            std::cout<<"[INFO] : ";
            break;
        case ERROR:
            std::cout<<"[ERROR] : ";
            break;
        case FATAL:
            std::cout<<"[FATAL] : ";
            break;
        case DEBUG:
            std::cout<<"[DEBUG] : ";
            break;
    }
    std::cout<<TimeStamp::now().toString()<<" : " <<msg<<std::endl;
}
















