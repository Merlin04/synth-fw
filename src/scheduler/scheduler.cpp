#include "scheduler.hpp"
#include "kscan/kscan_gpio_direct.hpp"
#include "kscan/velocity.hpp"

// This is a scheduler instance shared between velocity and kscan_gpio_direct
Scheduler<int> scheduler([](int& i) {
    if(i < 0) {
        kscan_direct_read();
    } else {
        velocity_scheduler_cb(i);
    }
});