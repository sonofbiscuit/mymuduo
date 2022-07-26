cmake_minimum_required(VERSION 3.22)
project(mymuduo)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(mymuduo
        TimeStamp.cpp
        TimeStamp.h EventLoop.cpp EventLoop.h nonCopyable.h currentThread.cpp currentThread.h Channel.cpp Channel.h Logger.cpp Logger.h Poller.cpp Poller.h EPollPoller.cpp EPollPoller.h)
