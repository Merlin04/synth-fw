#pragma once
#include "TeensyTimerTool.h"
#include "TimerModules/GPT/GPTChannel.h"
#include <list>

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

    void _timer_cb() {
        noInterrupts();
        // run the first one in the list
        auto j = jobs.front();
        jobs.pop_front();
        // Serial.printf("timer callback %d %d %d\n", j.param, j.run_at, micros());
        work(j.param);
        if(!jobs.empty()) {
            // Serial.printf("triggering from callback data %d t %d\n", jobs.front().param, jobs.front().run_at - micros());
            timer.timerChannel->setPeriod(jobs.front().run_at - micros());
            reinterpret_cast<TeensyTimerTool::GptChannel *>(timer.timerChannel)->regs->CR |= GPT_CR_EN;
            
        } else {
            // Serial.println("no more jobs");
            _cb_running = false;
        }
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
        // auto max_period = timer.getMaxPeriod();
        // Serial.printf("timer max period %f\n", max_period);
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
        
        noInterrupts();
        // Serial.println("yeag");
        if(!_cb_running || isAtFront) {
            // Serial.printf("triggering %d\n", run_at - micros());
            _cb_running = true;
            timer.trigger(run_at - micros());
        }
        interrupts();
    }

    void cancel(TParam param) {
        noInterrupts();
        for(auto it = jobs.begin(); it != jobs.end(); it++) {
            if(it->param == param) {
                jobs.erase(it);
                if(_cb_running && it == jobs.begin()) {
                    timer.stop();
                    _cb_running = false;
                    if(!jobs.empty()) {
                        timer.trigger(jobs.front().run_at - micros());
                    }
                }
                break;
            }
        }
        interrupts();
    }

    ~Scheduler() {
        timer.end();
    }

    friend void test_scheduler();
};

extern Scheduler<int> scheduler;