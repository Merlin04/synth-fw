#include "velocity.hpp"
#include "kscan_gpio_matrix.hpp"
#include "scheduler/scheduler_thread.hpp"
#include "hardware/ctrl_keys.hpp"

struct KeyState {
    uint32_t time = 0;
    bool measuring : 1;
    bool from_timeout : 1;
    uint8_t velocity = 0; // midi uses values 0-127, we can just divide this by 2 to convert to midi value
                          // maybe we can also use high resolution velocity prefix?
                          // https://www.midi.org/midi/specifications/midi1-specifications/midi-1-addenda/high-resolution-velocity-prefix
};

static struct KeyState key_states[MATRIX_LEN / 2];

velocity_callback_t velocity_callback;

void velocity_configure(velocity_callback_t callback) {
    velocity_callback = callback;
}

enum KeyType {
    KEY_TYPE_UPPER,
    KEY_TYPE_LOWER,
    KEY_TYPE_CTRL
};

static KeyType key_type(uint8_t row, uint8_t _column) {
    return row == 12 ? KEY_TYPE_CTRL : row % 2 == 0 ? KEY_TYPE_UPPER : KEY_TYPE_LOWER;
}
static void get_key_pos(uint8_t row, uint8_t column, uint8_t* out_r, uint8_t* out_c) {
    *out_r = row / 2;
    *out_c = column;
}

/*
 * summary of algorithm:
 * when upper pressed, reset velocity, set time, and start timeout timer
 * when lower pressed OR timeout, if key velocity unset, calculate/set velocity and do callback(set, velocity)
 *                                  otherwise, do callback with preset velocity
 * when lower released, callback(unset)
 * when upper released, don't do anything
 */

#define VELOCITY_TIMEOUT 300'000 // 150ms
#define VELOCITY_TIMEOUT_VALUE 50 // TODO: tune this value

SchedulerThread<int> scheduler = SchedulerThread<int>([](int& index) {
    Serial.println("timeout");
    struct KeyState* state = &key_states[index];
    state->velocity = VELOCITY_TIMEOUT_VALUE;
    state->from_timeout = true;
    state->measuring = false;

    uint8_t r, c;
    get_key_pos(index / COLS_LEN, index % COLS_LEN, &r, &c);
    velocity_callback(r, c, state->velocity, true);
});

void velocity_init() {
    scheduler.init();
}

void velocity_kscan_handler(uint8_t row, uint8_t column, bool pressed) {
//    Serial.printf("velocity_kscan_handler: row: %d, column: %d, pressed: %d\n", row, column, pressed);
    KeyType type = key_type(row, column);
    if(type == KEY_TYPE_CTRL) {
        auto key = static_cast<CtrlKey>(column);
        if(pressed) ctrl_keys_evt.emit(key);
        return;
    }

    uint8_t r, c;
    get_key_pos(row, column, &r, &c);
    uint8_t index = r * COLS_LEN + c;
    struct KeyState* state = &key_states[index];

    if(type == KEY_TYPE_UPPER) {
        if(pressed) {
            // Serial.println("upper pressed");
            state->time = micros();
            state->measuring = true;
            if(scheduler.cancel(index)) { // if someone double taps without clicking the bottom switch
                // things are broken here!!!
                Serial.println("aaaa");
                velocity_callback(r, c, state->velocity, true);
                velocity_callback(r, c, state->velocity, false); // TODO does this actually do anything useful????
            }
            scheduler.schedule(VELOCITY_TIMEOUT, index);
        } else {
            // Serial.println("upper released");
            if(state->from_timeout) {
                velocity_callback(r, c, state->velocity, false);
                state->from_timeout = false;
            }
        }
    } else if(!state->from_timeout) {
        if(pressed) {
            // Serial.println("lower pressed");
            if(state->measuring) {
                scheduler.cancel(index);
                uint32_t delay = micros() - state->time;
                Serial.printf("delay: %d\n", delay);
                state->measuring = false;
                // actually calculate the velocity - it should be within 0-127 (127 = highest velocity, so lowest delay)
                // for now we'll use a linear velocity curve, but something else might be good to implement in the future
                // state->velocity = (uint8_t) (127 * (1 - (float) delay / VELOCITY_TIMEOUT));
                state->velocity = ((((uint32_t)127) * (VELOCITY_TIMEOUT - delay)) / VELOCITY_TIMEOUT);
            }
            velocity_callback(r, c, state->velocity, true);
        } else {
            // Serial.println("lower released");
            velocity_callback(r, c, state->velocity, false);
        }
    }
}