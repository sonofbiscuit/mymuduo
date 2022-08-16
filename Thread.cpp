#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>

std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false)
    , joined_(false)
    , tid_(0)
    , func_(std::move(func))  // 右值引用，func是个左值，移动构造为func_
    , name_(name)
{
    setDefaultName();   // 设置默认名称
}

Thread::~Thread()
{
    if (started_ && !joined_)
    {
        thread_->detach();     // thread类提供了设置分离线程的方法 线程运行后自动销毁（非阻塞）
    }
}

void Thread::start()     // 一个Thread对象 记录的就是一个新线程的详细信息
{
    started_ = true;
    sem_t sem;  // 信号量
    sem_init(&sem, false, 0);     // false指的是 不设置进程间共享
    // 开启线程
    thread_ = std::shared_ptr<std::thread>(new std::thread([&]() {
        tid_ = CurrentThread::tid();        // 获取线程的tid值
        sem_post(&sem);   // 线程tid创建完毕后，调用sem_post来解除sem_wait的阻塞方法
        func_();           // 开启一个新线程 专门执行该线程函数
    }));

    // 这里必须等待获取上面新创建的线程的tid值
    sem_wait(&sem);   // 要保证线程完全开启在结束
}

/**
 * C++ std::thread 中join()和detach()的区别：https://blog.nowcoder.net/n/8fcd9bb6e2e94d9596cf0a45c8e5858a
 * join和detch的区别
 * 在声明一个thread对象之后，detch和join都可以用来启动被调线程，区别在于是否阻塞当前主调线程
 * join：主调线程阻塞，等待被调线程终止，然后主调线程回收被调线程资源，并继续运行
 * detch：主调线程继续运行，被调线程驻留后台运行，主调线程无法再取得该被调线程的控制权。
 *        当主调线程结束时，由运行时库负责清理与被调线程相关的资源。
*/
void Thread::join()
{
    joined_ = true;
    thread_->join();
}

// 默认名称
void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if (name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}