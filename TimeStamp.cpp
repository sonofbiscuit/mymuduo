//
// Created by 张庆 on 2022/7/24.
//
#include <time.h>
#include "TimeStamp.h"
#include<iostream>

TimeStamp::TimeStamp():microSecondsSinceEpoch_(0) {}

TimeStamp::TimeStamp(int64_t microSecondsSinceEpoch):
microSecondsSinceEpoch_(microSecondsSinceEpoch) {}

TimeStamp TimeStamp::now() {
    return TimeStamp(time(NULL));
}

std::string TimeStamp::toString() const {
    char buffer[128] = {0};
    tm *tm_cur = localtime(reinterpret_cast<const time_t *>(&microSecondsSinceEpoch_));
    snprintf(buffer, 128, "%4d/%02d/%02d %02d:%02d:%02d",
            tm_cur->tm_year+1900,
            tm_cur->tm_mon +1,
            tm_cur->tm_mday,
            tm_cur->tm_hour,
            tm_cur->tm_min,
            tm_cur->tm_sec);
    return buffer;
}

/**
int main(){
    std::cout<<TimeStamp::now().toString();
    return 0;
}/