//
// Created by 张庆 on 2022/7/24.
//

#include <sys/epoll.h>

#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"


const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;


Channel::Channel(EventLoop *loop, int fd) :
        loop_(loop),
        fd_(fd),
        events_(0),
        revents_(0),
        index_(-1),
        tied_(false) {}


Channel::~Channel() {}

/** channel 的 tie()方法， TcpConnection => Channel 时候调用
 *
 * TcpConnection 中注册了Channel对应的回调函数，传入的回调函数均为TcpConnection对象的成员方法
 * 因此可以说明: Channel的结束一定早于 TcpConnection对象
 * 用Tie可以解决 Channel对象 的生命周期问题，不让其提前释放，
 * 从而保证TcpConnection对象能够在 Channel对象之后销毁
 * */

void Channel::tie(const std::shared_ptr<void> &obj) {
    tie_ = obj;
    tied_ = true; // 绑定这个obj
}


/**
 * update
 * 当改变了 Channel 所表示的fd代表的事件后，update再负责去Poller中修改epoll_ctl
 * 修改 epoll_ctl 的绑定事件
 * */
void Channel::update() {
    // 通过当前 Channel 所属的 EventLoop 来找Poller修改
    // eventLoop 在 Channel 和 Poller 中充当桥梁的功能
    // Channel::update -> eventLoop::update -> Poller::update
    loop_->updateChannel(this);  // Channel 通常不直接和 Poller 交互
}

// 在 Channel 所属的 EventLoop 中，把当前 Channel 移除掉
void Channel::remove() {  // remove this
    loop_->removeChannel(this);
}

/**
 * handleEvent
 * 调用 handleEventWithGuard
 * */
void Channel::handleEvent(TimeStamp receiveTime) {
    if (tied_) {
        std::shared_ptr<void> guard = tie_.lock();  // 尝试
        if (guard) {  // 成功
            handleEventWithGuard(receiveTime);
        }
    } else {
        // TODO: tied_ 在这里体现的作用？
        handleEventWithGuard(receiveTime);
    }
}

// 根据fd上发生的事件，来判断返回的事件是什么
void Channel::handleEventWithGuard(TimeStamp receiveTime){  // 根据fd上发生的事件判断返回的事件是什么
    LOG_INFO("channel handleEvent revents:%d\n", revents_);
    // 关闭
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) // 当TcpConnection对应Channel 通过shutdown 关闭写端 epoll触发EPOLLHUP
    {
        if (closeCallback_)
        {
            closeCallback_();
        }
    }
    // 错误
    if (revents_ & EPOLLERR)
    {
        if (errorCallback_)
        {
            errorCallback_();
        }
    }
    // 读
    if (revents_ & (EPOLLIN | EPOLLPRI))
    {
        if (readCallback_)
        {
            readCallback_(receiveTime);
        }
    }
    // 写
    if (revents_ & EPOLLOUT)
    {
        if (writeCallback_)
        {
            writeCallback_();
        }
    }
}






