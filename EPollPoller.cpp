//
// Created by 张庆 on 2022/7/26.
//

#include <errno.h>
#include <unistd.h>   // 方便调用lunux系统服务
#include <iostream>
#include <string>

#include "Logger.h"
#include "EPollPoller.h"
#include "Channel.h"



// Channel 的成员变量index_ 默认初始化为-1
const int kNew = -1;   // 标记Channel 还未添加到Poller
const int kAdded = 1;  // 标记Channel已经添加到了Poller
const int kDeleted = 2;  // 表示Channel已经从Poller删除

EPollPoller::EPollPoller(EventLoop *loop):
    Poller(loop),
    epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
    events_(kInitEventSize)
    {
    //  成功时，返回一个非负文件描述符。
    //  发生错误时，返回-1，并且将errno设置为指示错误
        if(epollfd_ < 0){
            LOG_FATAL("epoll create1 error: %d\n", epollfd_);
        }
    }


EPollPoller::~EPollPoller() {
    ::close(epollfd_);
}

TimeStamp EPollPoller::poll(int timeoutMs, channelList *activeChannels) {
    // 设置为LOG_DEBUG, 高并发时，关闭debug，以提升效率
    LOG_INFO("function: %s => fd total count: %lu\n",__FUNCTION__ ,channels_.size());

    // epoll_wait 等待和监听事件，若发生，则返回发生事件的数目
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;  // errno 是记录系统的最后一次错误代码
    TimeStamp now(TimeStamp::now());

    if(numEvents>0){  // 有事件发生,此时这些事件的fd已经存在了events_中
        LOG_INFO("%d events happened\n", numEvents);  // LOG_DEBUG
        fillActiveChannels(numEvents, activeChannels);  // 保存活跃的事件
        if(numEvents == events_.size()){
            events_.re
        }
    }else if(numEvents == 0){  // 超时
        LOG_DEBUG("%s time out! \n", __FUNCTION__ );
    }else{  // 无事件发生
        if(saveErrno != EINTR){  // 如果错误为EINTR表示在读/写的时候出现了中断错误
            error = saveErrno;
            LOG_ERROR("EPollPoller::poll() error!");
        }
    }
}


void EPollPoller::updateChannel(Channel *channel) {

}

void EPollPoller::removeChannel(Channel *channel) {

}

// 保存活跃的任务
void EPollPoller::fillActiveChannels(int numEvents, channelList *activeChannels) const {
    for(int i = 0;i< numEvents;++i){
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);  // 所有发生事件的列表
    }
}















