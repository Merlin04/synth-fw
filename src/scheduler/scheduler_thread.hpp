// scheduler impl based on threading, not interrupts
// (so it supports multiple schedulers!) plus maybe won't randomly segfault

#pragma once

#include <TeensyThreads.h>
#include <Arduino.h>
#include <list>
#include "kscan/kscan_gpio_matrix.hpp"
#include "kscan/velocity.hpp"
#include "util/thread.hpp"

template<typename TParam>
class SchedulerThread : public Thread<SchedulerThread<TParam>> {
private:
    struct Job {
        uint32_t run_at;
        TParam param;
    };

    std::list<Job> jobs;
    Threads::Mutex jobs_mutex;
    using WorkFunction = void (*)(TParam&);
    volatile WorkFunction work;

public:
    explicit SchedulerThread(WorkFunction work) : work(work) {}

    void schedule(uint32_t delay_us, TParam param) {
        auto c = micros();
        uint32_t run_at = c + delay_us;
        schedule_at(run_at, c, param);
    }

    void schedule_at(uint32_t run_at, uint32_t ra_relative_to_ts, TParam param) {
        Threads::Scope m(jobs_mutex);
        auto it = jobs.begin();

        if(run_at < ra_relative_to_ts) {
            // we looped over, so increase iterator until the run_at is less than the previous item
            uint32_t prev = it->run_at;
            while(it != jobs.end() && it->run_at >= prev) {
                prev = it->run_at;
                it++;
            }
        }

        while(it != jobs.end() && it->run_at < run_at) {
            it++;
        }
        jobs.insert(it, { run_at, param });
    }

    bool cancel(TParam param) {
        bool found = false;
        Threads::Scope m(jobs_mutex);
        for(auto it = jobs.begin(); it != jobs.end(); it++) {
            if(it->param != param) continue;

            found = true;
            jobs.erase(it);
            break;
        }

        return found;
    }

private:
    [[noreturn]] void thread_fn() {
        while(true) {
            jobs_mutex.lock();
            auto j = jobs.front();
            if(j.run_at - micros() <= 0) {
                jobs.pop_front();
                jobs_mutex.unlock();
                work(j.param);
            } else {
                jobs_mutex.unlock();
                Threads::yield(); // something isn't going to be added during this thread's execution
            }
        }
    }
    friend class Thread<SchedulerThread<TParam>>;

    void thread_init() {
        Thread<SchedulerThread<TParam>>::thread_init();
    }

public:
    void init() {
        thread_init();
    }
};

// This is a scheduler instance shared between velocity and kscan_gpio_direct
// (shared because why not, saves a thread)
inline SchedulerThread<int> scheduler = SchedulerThread<int>([](int& i) {
    if(i < 0) {
        kscan_matrix_read();
    } else {
        velocity_scheduler_cb(i);
    }
});