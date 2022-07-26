//
// Created by 张庆 on 2022/7/25.
//

#include "Poller.h"
#include "Channel.h"


// 默认构造函数就是绑定对应的 EventLoop
Poller::Poller(EventLoop *loop):ownerLoop_(loop) {}

// 查找是否存在channel
bool Poller::hasChannel(Channel *channel) const {
    auto it = channels_.find(channel->fd()); // 根据fd查找是否有对应的channel
    return (it != channels_.end() && it->second == channel);
}

