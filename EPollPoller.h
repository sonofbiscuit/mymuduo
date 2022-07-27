//
// Created by 张庆 on 2022/7/26.
//

#pragma oncec

#include <vector>
#include <sys/epoll.h>

#include "Poller.h"
#include "TimeStamp.h"

/**
 * 要把A放到B里，永远只需要三步，把fd放到epoll里也不例外：
 * 打开epoll
 * 把fd扔进去
 * 等待
 *  int epoll_create(int size)；
    int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)；
    int epoll_wait(int epfd, struct epoll_event * events, int maxevents, int timeout);
 * */

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

    int epollfd_;    // 代表新创建的epoll实例的文件描述符(event_create函数的返回值)
    EventList events_;   // 用于存放epoll_wait返回的所有发生的事件的文件描述符事件集合

};












