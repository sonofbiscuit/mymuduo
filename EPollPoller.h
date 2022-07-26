//
// Created by 张庆 on 2022/7/26.
//

#pragma oncec

#include <vector>
#include <sys/epoll.h>

#include "Poller.h"
#include "TimeStamp.h"


/**
 * epoll的功能:
 * epoll_create
 * epoll_wait 检测
 * epoll_ctl  绑定
 * */
class EPollPoller: public Poller{
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller();

    // 基类的抽象方法
    TimeStamp poll(int timeoutMs, channelList *activeChannels);
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);

private:
    // 对于每个epoll，需要定义其 epoll_event 数组的长度
    static const int kInitEventSize = 16;

    /**
     * fillActive 将 epoll返回的活跃channel 返回给activeChannels
     * epoll 返回的 fd保存在epoll_events数组中
     * */
    void fillActiveChannels(int numEvents, channelList *activeChannels) const;

    /**
     * 更新channel，就是更新状态的绑定
     * epoll_ctl
     * */
    void update(int operation, Channel *channel);   //operation  是epoll_ctl的三种操作类型

    /**
     * struct epoll_event{
     *      uint32_t event;
     *      epoll_data_t data;
     * };
     *
     * typedef epoll_data_t{
     *      void *ptr;
     *      int fd;
     *      uint32_t u32;
     *      uint64_t u64;
     * }epoll_data_t
     * */
    using EventList = std::vector<epoll_event>;

    int epollfd_;    // event_create创建返回的fd保存在epollfd_
    EventList events_;   // 用于存放epoll_wait返回的所有发生的事件的文件描述符事件集合

};












