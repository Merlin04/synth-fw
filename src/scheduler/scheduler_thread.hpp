// scheduler impl based on threading, not interrupts
// (so it supports multiple schedulers!) plus maybe won't randomly segfault

#pragma once

#include <TeensyThreads.h>
#include <Arduino.h>
#include <list>
#include "kscan/kscan_gpio_matrix.hpp"
#include "kscan/velocity.hpp"
#include "util/thread.hpp"

//#define SCHEDULER_DEBUG

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
#ifdef SCHEDULER_DEBUG
        Serial.printf("schedule_at: %d, %d\n", run_at, ra_relative_to_ts);
#endif
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
#ifdef SCHEDULER_DEBUG
            Serial.printf("scheduler thread_fn: %d\n", jobs.size());
            if(!jobs.empty()) {
                Serial.printf("scheduler thread_fn: front run_at: %d (currently %d)\n", jobs.front().run_at, micros());
            }
#endif
            if(!jobs.empty() && ((int64_t) jobs.front().run_at) - ((int64_t) micros()) <= 0) {
#ifdef SCHEDULER_DEBUG
                Serial.printf("scheduler work\n");
#endif
                auto j = jobs.front();
#ifdef SCHEDULER_DEBUG
                Serial.printf("%d\n", j.param);
#endif
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
#ifdef SCHEDULER_DEBUG
    Serial.printf("main scheduler work: %d\n", i);
#endif
    if(i < 0) {
        kscan_matrix_read();
    } else {
        velocity_scheduler_cb(i);
    }
});