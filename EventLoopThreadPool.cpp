#include <memory>

#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg)
    : baseLoop_(baseLoop)
    , name_(nameArg)
    , started_(false)
    , numThreads_(0)
    , next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
    // Don't delete loop, it's stack variable
    // EventLoopThread::threadFunc()开始时会创建一个loop
    // 结束后会自动释放
}

void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
    started_ = true;

    for(int i = 0; i < numThreads_; ++i)    // 开启对应数量的EventLoopThread
    {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);  // 对线程名称的一个初始化操作
        EventLoopThread *t = new EventLoopThread(cb, buf);   // 创建线程，同时构造函数去给线程绑定回调函数threadFunc
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startLoop());             // 底层创建线程 绑定一个新的EventLoop 并返回该loop的地址
        // startLoop 创建一个对应的EventLoop并返回对应的指针
    }

    if(numThreads_ == 0 && cb)                                      // 整个服务端只有一个线程运行baseLoop
    {
        cb(baseLoop_);
    }
}

// 如果工作在多线程中，baseLoop_(mainLoop)会默认以轮询的方式分配Channel给subLoop
// 如果对fd产生了一些事件，那么会通过 getNextLoop 将这些描述符fd进行下发
EventLoop *EventLoopThreadPool::getNextLoop()   // 获取子loop
{
    // 如果只设置一个线程 也就是只有一个mainReactor 无subReactor 那么轮询只有一个线程 getNextLoop()每次都返回当前的baseLoop_
    EventLoop *loop = baseLoop_;   // EventLoop *baseLoop_;

    // loops_  存放所有subloop的指针
    if(!loops_.empty())             // 通过轮询获取下一个处理事件的loop
    {
        loop = loops_[next_];
        ++next_;
        if(next_ >= loops_.size())
        {
            next_ = 0;
        }
    }

    return loop;  // 取出一个eventloop 并返回
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops()
{
    if(loops_.empty())  // 只有主loop  或者只剩主
    {
        return std::vector<EventLoop *>(1, baseLoop_);
    }
    else
    {
        return loops_;
    }
}