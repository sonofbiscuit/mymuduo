//
// Created by 张庆 on 2022/7/24.
//

#ifndef MYMUDUO_NONCOPYABLE_H
#define MYMUDUO_NONCOPYABLE_H


/**
 * 不允许class被拷贝
 * 当一个类继承了nonCopyable类，那么该对象派生类无法进行拷贝构造和赋值构造
 * TODO： 移动构造呢？需不需要delete
 * */

class nonCopyable{
public:
    // 拷贝构造
    nonCopyable(const nonCopyable &cp) = delete;
    // 赋值构造
    nonCopyable &operator=(const nonCopyable &cp) = delete;
    // 移动构造
    nonCopyable &operator=(const nonCopyable &&cp) = delete;

    nonCopyable() = default;
    ~nonCopyable() = default;
};





#endif //MYMUDUO_NONCOPYABLE_H
