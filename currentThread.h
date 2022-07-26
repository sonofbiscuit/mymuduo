//
// Created by 张庆 on 2022/7/24.
//

#ifndef MYMUDUO_CURRENTTHREAD_H
#define MYMUDUO_CURRENTTHREAD_H

#include <unistd.h>
#include <sys/syscall.h>

// namespace 是开放的，可以在多个文件中添加
namespace currentThread {
    extern __thread int t_cachedTid; // 保存tid缓存
    // extern使得其可以全局访问，因为系统调用非常耗时

    void cacheTid();

    // 内联函数只在当前文件中起作用
    inline int tid(){
        if(__builtin_expect(t_cachedTid == 0, 0)){  // 等于0的概率很大
            // __builtin_expect 允许将最有可能的分之告诉编译器，让编译器告诉cpu提前加载该分支下的指令
            // __builtin_expect(EXP, N) 意思是EXP == N 的概率很大
            cacheTid();
        }
        return t_cachedTid;
    }
}


#endif //MYMUDUO_CURRENTTHREAD_H
