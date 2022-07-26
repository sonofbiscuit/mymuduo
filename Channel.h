//
// Created by 张庆 on 2022/7/24.
//

#pragma once

#include<memory>
#include<functional>

#include "nonCopyable.h"
#include "Channel.h"
#include "TimeStamp.h"

class EventLoop;

/**
 * EventLoop, Channel, Poller之间的关系
 * Reactor 模型上对应多路事件发生器
 * file descriptor could be a socket
 * Channel 封装了 socketfd 和其感兴趣的event ，如 读/写/错误
 * */

class Channel:nonCopyable {  // 继承防止拷贝
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(TimeStamp)>;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    /** fd 得到 poller 通知后，调用EventLoop::loop() 来执行handleEvent
     * handleEvent 是 Channel 的核心
     */
    void handleEvent(TimeStamp receiveTime);

    /**
     * 设置回调函数的对象
     * */
    void setReadCallback(ReadEventCallback cb){readCallback_ = std::move(cb);}
    void setWriteCallback(EventCallback cb){writeCallback_ = std::move(cb);}
    void setErrorCallback(EventCallback cb){errorCallback_ = std::move(cb);}
    void setCloseCallback(EventCallback cb){closeCallback_ = std::move(cb);}

    // 防止当channel被手动remove掉， channel还在继续执行回调
    //TODO： TCP
    void tie(const std::shared_ptr<void>&);

    int fd() const{return fd_;}
    int events() const{return events_;}
    void set_revents(int ev){revents_ = ev;}

    /**
     * 设置fd相应的事件状态，相当于 epoll_ctl 中添加和删除
     */
    void enableReading(){events_ |= kReadEvent; update();}
    void disableReading(){events_ &= ~kReadEvent; update();}
    void enableWriting(){events_ |= kWriteEvent; update();}
    void disableWriting(){events_ &= ~kWriteEvent; update();}
    void disableAll(){events_ = kNoneEvent; update();}

    /**
     * 返回fd当前的事件状态
     * */
    bool isWriting() const{return events_ & kWriteEvent;}  // 根据标记位判断
    bool isReading() const{return events_ & kReadEvent;}
    bool isNoneEvent() const{return events_ == kNoneEvent;}

    int index(){return index_;}
    void set_index(int index){index_ = index;}

    /**
     * one loop per thread
     * */
     EventLoop *ownerLoop(){return loop_;}
     void remove();

private:
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback errorCallback_;
    EventCallback closeCallback_;

    // 标记位
    static const int kNoneEvent; // disableAll 的标记位
    static const int kReadEvent;
    static const int kWriteEvent;


    EventLoop* loop_;  // 事件循环
    const int fd_;  // poller监听的对象
    int events_; // 注册fd感兴趣的事件
    int revents_;  // poller返回的具体发生的事件
    int index_;  // used by poller

    std::weak_ptr<void> tie_;  // muduo中的弱回调，改变对象的生命周期问题
    bool tied_;  // a对象调用b对象，b对象还没销毁，但是a马上要销毁，那么来延长生命周期

    // Channel::update() => EventLoop::updateChannel() => Poller::updateChannel()
    void update();


    void handleEventWithGuard(TimeStamp receiveTime);
};

