//
// Created by 张庆 on 2022/7/24.
//

#pragma once

#include <string>
#include <iostream>

class TimeStamp {
public:
    TimeStamp();
    explicit TimeStamp(int64_t microSecondsSinceEpoch);
    static TimeStamp now();
    std::string toString() const;
private:
    int64_t microSecondsSinceEpoch_;
};


