#pragma once
#include "TeensyTimerTool.h"
#include "TimerModules/GPT/GPTChannel.h"
#include <list>

// #define SCHEDULER_DEBUG

// simple scheduler class to allow you to use a single timer to schedule executions of a function with a given parameter of some type
template<typename TParam>
class Scheduler {
    private:

    struct Job {
        uint32_t run_at;
        TParam param;
    };
    TeensyTimerTool::OneShotTimer timer = TeensyTimerTool::OneShotTimer(TeensyTimerTool::GPT2);
    std::list<Job> jobs;
    using WorkFunction = void (*)(TParam&);
    volatile WorkFunction work;
    volatile bool _cb_running = false;
    volatile bool _inside_cb = false;

    TeensyTimerTool::errorCode _triggerTimer(uint32_t t) {
        // Serial.printf("triggering timer %d\n", t);
        // if we're inside the _timer_cb, we need to trigger a different way since we can't overwrite interrupt flags
        if(_inside_cb) {
            // if(t == 0) {
            //     Serial.printf("ok sure\n");
            //     return TeensyTimerTool::errorCode::OK; // we're inside the cb, we'll get to the job right after
            // }
            // we'll get to the job right after
            return TeensyTimerTool::errorCode::OK;
        } else {
            if(t == 0) {
                Serial.printf("aaaaaa\n");
                t = 1; // one nanosecond won't hurt anyone
            }
            return timer.trigger(t);
        }
    }

    void _timer_cb() {
        noInterrupts();
        _inside_cb = true;
        // run the first one in the list
        auto j = jobs.front();
        if(j.param != -1) Serial.printf("timer callback %d %d %d\n", j.param, j.run_at, micros());
        jobs.pop_front();
#ifdef SCHEDULER_DEBUG
        Serial.printf("timer callback %d %d %d\n", j.param, j.run_at, micros());
#endif
        work(j.param);
        if(!jobs.empty()) {
#ifdef SCHEDULER_DEBUG
            Serial.printf("triggering from callback data %d t %d\n", jobs.front().param, jobs.front().run_at - micros());
#endif
            // _triggerTimer(jobs.front().run_at - micros());      
            timer.timerChannel->setPeriod(jobs.front().run_at - micros());
            reinterpret_cast<TeensyTimerTool::GptChannel *>(timer.timerChannel)->regs->CR |= GPT_CR_EN;
        } else {
#ifdef SCHEDULER_DEBUG
            Serial.println("no more jobs");
#endif
            _cb_running = false;
        }
        _inside_cb = false;
        interrupts();
    }

    public:
    Scheduler(WorkFunction work) : work(work) {
    }

    void init_timer() {
        // separate so we don't have to call it on construction
        TeensyTimerTool::attachErrFunc(TeensyTimerTool::ErrorHandler(Serial));
        timer.begin([this]() {
            this->_timer_cb();
        });
    #ifdef SCHEDULER_DEBUG
        auto max_period = timer.getMaxPeriod();
        Serial.printf("timer max period %f\n", max_period);
    #endif
    }

    void schedule(uint32_t delay_us, TParam param) {
        auto c = micros();
        uint32_t run_at = c + delay_us;
        schedule_at(run_at, c, param);
    }

    void schedule_at(uint32_t run_at, TParam param) {
        schedule_at(run_at, micros(), param);
    }

    void schedule_at(uint32_t run_at, uint32_t ra_relative_to_ts, TParam param) {
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
        bool isAtFront = it == jobs.begin();
        jobs.insert(it, { run_at, param });

        if(param != -1) {
            Serial.printf("schedule %d %d %d %d\n", param, run_at, micros(), run_at - micros());
        }
        
        noInterrupts();
        if(!_cb_running || isAtFront) {
#ifdef SCHEDULER_DEBUG
            Serial.printf("triggering %d\n", run_at - micros());
#endif
            _cb_running = true;
            auto t = run_at - micros();
            auto err = _triggerTimer(t);
            if(err != TeensyTimerTool::errorCode::OK) {
                // print params
                Serial.printf("schedule error %d %d %d %d %d\n", err, run_at, micros(), t, param);
            }
        }
        interrupts();
    }

    bool cancel(TParam param) {
        noInterrupts();
        bool found = false;
        for(auto it = jobs.begin(); it != jobs.end(); it++) {
            if(it->param == param) {
                found = true;
                jobs.erase(it);
                if(_cb_running && it == jobs.begin()) {
                    timer.stop();
                    _cb_running = false;
                    if(!jobs.empty()) {
                        auto err = _triggerTimer(jobs.front().run_at - micros());
                        if(err != TeensyTimerTool::errorCode::OK) {
                            // print params
                            Serial.printf("cancel error %d %d %d %d %d\n", err, jobs.front().run_at, micros(), jobs.front().run_at - micros(), jobs.front().param);
                        }
                    }
                }
                break;
            }
        }
        interrupts();
        return found;
    }

    ~Scheduler() {
        timer.end();
    }

    friend void test_scheduler();
};

extern Scheduler<int> scheduler;