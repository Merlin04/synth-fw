#include "scheduler_thread.hpp"
#include "kscan/kscan_gpio_matrix.hpp"
#include "kscan/velocity.hpp"

// This is a scheduler instance shared between velocity and kscan_gpio_direct
// (shared because why not, saves a thread)
SchedulerThread<int> scheduler = SchedulerThread<int>([](int& i) {
    if(i < 0) {
        kscan_matrix_read();
    } else {
        velocity_scheduler_cb(i);
    }
});