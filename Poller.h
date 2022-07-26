//
// Created by 张庆 on 2022/7/25.
//

#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>

#include "nonCopyable.h"
#include "TimeStamp.h"

class Channel;
class EventLoop;

/**
 * 多路事件分发器的核心IO复用模块
 * Poller是个抽象类
 * */
class Poller {
public:
    using channelList = std::vector<Channel *>;  // 作为 Poller 的传入

    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    /** 给所有的IO复用保留统一的接口*/
    virtual TimeStamp poll(int timeoutMs, channelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;

    // 判断当前channel是否在预设的 poll 中
    bool hasChannel(Channel *channel) const;

    // EVentPool 获取默认的IO复用
    static Poller *newDefaultPoller(EventLoop* loop);

protected:
    // r-b tree 保存的 fd
    using channelMap = std::unordered_map<int, Channel *>;  // fd和对应的channel
    channelMap channels_;

private:
    // 当前 Poller 所属于的EventLoop
    EventLoop* ownerLoop_;


};


