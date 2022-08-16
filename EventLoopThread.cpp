#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                 const std::string &name)
    : loop_(nullptr)
    , exiting_(false)
    , thread_(std::bind(&EventLoopThread::threadFunc, this), name)   // Thread类创建的一个thread_对象是一个threadfunc
    , mutex_()
    , cond_()
    , callback_(cb)  // 线程初始化的一个回调函数
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}

EventLoop *EventLoopThread::startLoop()
{
    thread_.start(); // 启用底层线程Thread类对象thread_中通过start()创建的线程
    // 开启一个线程并且让线程拥有自己的tid

    EventLoop *loop = nullptr;
    {   // 临界区加锁
        std::unique_lock<std::mutex> lock(mutex_);
        while(loop_ == nullptr)  // 为空，说明当前线程还没有EventLoop
        {
            cond_.wait(lock);  // 条件变量阻塞，直至有loop
        }
        loop = loop_;
    }
    return loop;
}

// 下面这个方法 是在单独的新线程里运行的
void EventLoopThread::threadFunc()
{
    // 线程开始的时候就会产生一个新的loop，是属于当前线程的
    EventLoop loop; // 创建一个独立的EventLoop对象 和上面的线程是一一对应的 级one loop per thread

    if (callback_)   // 可以把线程开始时需要执行的操作放到callback中
    {
        callback_(&loop);
    }

    {
        // 通过条件变量和互斥锁的方式将eventloop对象 通过startloop返回过去
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;   // 绑定loop_指针
        cond_.notify_one();   // 通知阻塞的条件变量，就是上面的 cond_.wait(lock);
    }
    loop.loop();    // 执行EventLoop的loop() 开启了底层的Poller的poll()

    // =========执行下面的语句，说明loop要结束了，否则一直循环在loop.loop()
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}