#include "velocity.hpp"
#include "kscan_config.hpp"
#include "kscan_gpio.hpp"
#include "scheduler/scheduler.hpp"

/*
 * summary of algorithm:
 * when upper pressed, reset velocity, set time, and start timeout timer
 * when lower pressed OR timeout, if key velocity unset, calculate/set velocity and do callback(set, velocity)
 *                                  otherwise, do callback with preset velocity
 * when lower released, callback(unset)
 * when upper released, don't do anything
 */

void velocity_scheduler_cb(int& i) {
    Serial.println(i);
}

// simple function to test that scheduling and cancelling jobs works!
void test_scheduler() {
    Serial.println("starting scheduler test");
    // Scheduler<int> scheduler(velocity_scheduler_cb);

    Serial.println("scheduling 1");
    scheduler.schedule(1000000, 1);
    Serial.println("scheduling 2");
    scheduler.schedule(2000000, 2);
    Serial.println("scheduling 3");
    scheduler.schedule(3000000, 3);
    Serial.println("scheduling 4");
    scheduler.schedule(4000000, 4);

    // debug: print the jobs
    for(auto j : scheduler.jobs) {
        Serial.printf("job p: %d r: %d\n", j.param, j.run_at);
    }

    Serial.println("cancelling 2");
    scheduler.cancel(2);
    Serial.println("delaying");

    delay(1000);
    Serial.println("scheduling 5");
    scheduler.schedule(1000000, 5);
    Serial.println("delaying");

    delay(1'000'000);
    Serial.println("ending scheduler test");
}

struct KeyState {
    uint32_t t;
    uint32_t v;
};

static struct KeyState key_states[INST_MATRIX_LEN / 2];

typedef void(* velocity_callback_t) (uint8_t row, uint8_t column, uint32_t velocity, bool pressed);

velocity_callback_t velocity_callback;

void velocity_configure(velocity_callback_t callback) {
    velocity_callback = callback;
}

enum KeyType {
    KEY_TYPE_UPPER,
    KEY_TYPE_LOWER
};

#if INST_ROWS_LEN == 1
static KeyType key_type(uint8_t row, uint8_t column) {
    switch(column) {
        case 0: [[fallthrough]]
        case 1: [[fallthrough]]
        case 2:
            return KEY_TYPE_LOWER;
    }
    return KEY_TYPE_UPPER;
}
static void get_key_pos(uint8_t row, uint8_t column, uint8_t* out_r, uint8_t* out_c) {
    *out_r = 0;
    switch(column) {
        case 3: [[fallthrough]]
        case 2:
            *out_c = 2;
            break;
        case 4: [[fallthrough]]
        case 1:
            *out_c = 1;
            break;
        case 5: [[fallthrough]]
        case 0:
            *out_c = 0;
            break;
    }
}
#else
static KeyType key_type(uint8_t row, uint8_t column) {
    return row % 2 == 0 ? KEY_TYPE_UPPER : KEY_TYPE_LOWER;
}
static void get_key_pos(uint8_t row, uint8_t column, uint8_t* out_r, uint8_t* out_c) {
    *out_r = row / 2;
    *out_c = column;
}
#endif

void velocity_kscan_handler(uint8_t row, uint8_t column, bool pressed) {
    KeyType type = key_type(row, column);
    uint8_t r, c;
    get_key_pos(row, column, &r, &c);
    uint8_t index = r * INST_COLS_LEN + c;
    struct KeyState* state = &key_states[index];

    
}