//
// Created by 张庆 on 2022/7/24.
//

#include "currentThread.h"

namespace currentThread{
    /**
     * __thread是线程的局部存储设施，
     * 每个线程独有一份__thread，各个线程的值互不干扰
     * 通过cache 的 id 来获取 线程id
     * */
    __thread int t_cachedTid = 0;

    void cacheTid(){
        if(t_cachedTid == 0){
            // static_cast<pid_t>(::syscall(SYS_gettid)) 是获取tid的方式  线程标志符
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}

