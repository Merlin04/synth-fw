#pragma once

#include <TeensyThreads.h>

template<class T>
class Thread {
private:
    static void thread_wrapper(void* a) {
        auto pThis = static_cast<T*>(a);
        pThis->thread_fn();
    }
    void thread_init() {
        threads.addThread(thread_wrapper, this);
    }
    friend T;
};