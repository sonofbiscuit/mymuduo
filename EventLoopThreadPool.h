#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>

#include "noncopyable.h"

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;   // 线程初始化的回调

    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads) { numThreads_ = numThreads; }   // 设置线程数，
    // 之后会通过循环的方式创建和eventloop和thread绑定的对象

    void start(const ThreadInitCallback &cb = ThreadInitCallback());  // 传入一个cb

    // 如果工作在多线程中，baseLoop_(mainLoop)会默认以轮询的方式分配Channel给subLoop
    EventLoop *getNextLoop();  // 以轮循的方式给subLoop分配channel

    std::vector<EventLoop *> getAllLoops();   // 将所有的eventLoop通过vector进行返回

    bool started() const { return started_; }   // 当前是否启动
    const std::string name() const { return name_; }

private:
    EventLoop *baseLoop_; // 用户使用muduo创建的loop 如果线程数为1 那直接使用用户创建的loop 否则创建多EventLoop
    std::string name_;
    bool started_;
    int numThreads_;
    int next_; // 轮询的下标
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop *> loops_;
};