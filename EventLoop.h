//
// Created by 张庆 on 2022/7/24.
//

#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <mutex>
#include <memory>

#include "TimeStamp.h"
#include "nonCopyable.h"
#include "currentThread.h"

class Channel;
class Poller;

class EventLoop:nonCopyable {  // 继承了nonCopyable，防止拷贝
public:
    using functor = std::function<void()>;

    /**
     * 默认构造和析构
     * */
    EventLoop();
    ~EventLoop();

    /**
     * 事件循环函数loop()
     * 退出循环
     * */
    void loop();
    void quit_loop();

    /**
     * 在当前loop中执行callback
     * callback 丢到队列中
     * */
    void runInLoop(functor cb);
    // callback 放入到队列中，唤醒线程开始执行cb
    void queueInLoop(functor cb);

    /**
     * muduo 是个多 reactor 网络库， 也支持单reactor
     * 如果当前只有一个单reactor ，那么难以承受高并发。可以使用多核的优势
     * muduo 创建了多个线程，创建多个reactor，去进行事件循环处理。每个reactor都可以当作一个epoll wait
     *
     * 一个主reactor去处理当前请求的到来，进行接收，然后将返回的fd分发给子reactor进行处理用户连接请求中的数据
     *
     * ==WAKEUP==
     * 如果当前主reactor中有一个回调函数，需要交给子reactor进行执行
     * 但是当前子reactor没有事件发生，一直阻塞在epoll_wait阶段，所以主reactor没法将cb注册到子reactor之中
     * wakeup的作用就是取消子reactor的阻塞
     * */
    void wakeupThd();

    /**
     * EventLoop的一系列方法
     * EventLoop::updateChannel() => Poller::updateChannel()
     * */
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);

    /**
     * 判断当前EventLoop是否在自己线程
     */
    bool isLoopInThread(){
        // threadId_ 是为eventloop创建线程时的线程id， currentThread::tid()是获取当前线程id
        return threadId_ == currentThread::tid();
    }

private:
    const pid_t threadId_;  // 当前EventLoop的线程id
    std::atomic_bool looping_;  // 原子操作, 正在循环的标志
    std::atomic_bool quit_; // 原子操作，标识loop退出

    // poller
    TimeStamp  pollReturnTime_;  // poller返回发生事件的Channels的时间点
    std::unique_ptr<Poller> poller_;

    // 当loop创建一个Channel对象时，channel对象会处理唯一一个fd
    int wakeupFd_; // loop选择一个新用户的Channel时，会通过轮询算法选择一个subplot，通过该成员唤醒subloop处理channel
    std::unique_ptr<Channel> wakeupChannel_;  // 指向唤醒的channel

    // 记录所有的活跃channel
    using channelList = std::vector<Channel*>;
    channelList activeChannelList_;  // poller检测到当前有事件发生的所有channel

    std::atomic_bool callingPendingFunctors_;  // 标识当前loop是否有需要执行的回调操作
    std::vector<functor> pendingFunctors; // 存储loop的所有回调操作

    std::mutex mutex_;  // 互斥锁


    void doPendingFunctors();  // 执行存储的回调函数

    // 给eventfd返回的文件描述符wakeupFd_绑定的事件回调 当wakeup()时 即有事件发生时 调用handleRead()读wakeupFd_的8字节 同时唤醒阻塞的epoll_wait
    // handleRead是wakeup fd所绑定的一个回调
    // fd和回调函数的绑定就是通过channel
    /**
     * 我们是通过eventfd唤醒loop所在的线程，最终能返回一个文件描述符wakeupFd_
     * 给这个文件描述符绑定回调
     * 调用handleRead读取wakeupFd_的8字节，同时唤醒阻塞的epoll_wait
     * */
    void handleRead();
};


