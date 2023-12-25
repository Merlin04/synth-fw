#include "velocity.hpp"
#include "kscan_gpio_matrix.hpp"
#include "scheduler/scheduler_thread.hpp"
#include "hardware/ctrl_keys.hpp"
#include "util/log.hpp"

/*

keystate = {
    timer_state = None | Running | TimedOut
    top_ts = number
    velocity = number
    playing = bool
};

also use mutexes to access this data structure !!!
on the loop when key pressed you should lock it



on key press
    bottom one:
        press:
            if it timed out, don't do anything and clear the timeout flag
            if velocity timing job, cancel it
            if velocity is set, send that
            if the top level one is set, but velocity isn't, update velocity and send it
            if otherwise, this is error, so just set velocity to the default and send it
        release:
            stop note
    top one:
        press:
            if velocity is set, don't do anything (bottom would have been pressed before, so already actuated)
            set the timer, set the top start time
        release:
            clear velocity
            stop note if playing

*/

auto l = new Log<false>("velocity");

velocity_callback_t velocity_callback;

void velocity_configure(velocity_callback_t callback) {
    velocity_callback = callback;
}

enum class KeyType : uint8_t {
    upper,
    lower,
    ctrl
};

static KeyType key_type(const uint8_t row, uint8_t /*_column*/) {
    // for some reason clang-tidy doesn't like if I do it all in one line
    if(row == 12) {
        return KeyType::ctrl;
    }
    return row % 2 == 0 ? KeyType::upper : KeyType::lower;
    // return row == 12 ? KeyType::ctrl : row % 2 == 0 ? KeyType::upper : KeyType::lower;
}
static void get_key_pos(const uint8_t row, const uint8_t column, uint8_t* out_r, uint8_t* out_c) {
    *out_r = row / 2;
    *out_c = column;
}

// TODO: tune these values
#define VELOCITY_TIMEOUT 100'000 // 100ms
#define VELOCITY_TIMEOUT_VALUE 150

void scheduler_work(const uint8_t& index);
auto scheduler = SchedulerThread(scheduler_work);

void velocity_init() {
    scheduler.init();
}


enum class TimerState : uint8_t {
    none,
    running,
    timed_out
};

struct KeyState {
    TimerState timer_state = TimerState::none;
    uint32_t top_ts = 0;
    uint8_t velocity = 0;
    bool playing = false;
};

Threads::Mutex key_states_lock;
volatile static KeyState key_states[MATRIX_LEN / 2];

void send_press(const uint8_t r, const uint8_t c, volatile KeyState* state) {
    if(!state->playing) {
        state->playing = true;
        velocity_callback(r, c, state->velocity, true);
    }
}

void send_release(const uint8_t r, const uint8_t c, volatile KeyState* state) {
    if(state->playing) {
        state->playing = false;
        velocity_callback(r, c, 0, false);
    }
}

uint8_t get_index(const uint8_t row, const uint8_t column) {
    return row * COLS_LEN + column;
}

void scheduler_work(const uint8_t& index) {
    Threads::Scope m(key_states_lock);

    const auto state = &key_states[index];
    if(state->timer_state != TimerState::running) {
        return; // shouldn't be in this state
    }
    state->timer_state = TimerState::timed_out;
    state->velocity = VELOCITY_TIMEOUT_VALUE;

    send_press(index / COLS_LEN, index % COLS_LEN, state);
}
void velocity_kscan_handler(const uint8_t matrix_row, const uint8_t matrix_column, const bool pressed) {
    Threads::Scope m(key_states_lock);

    l->debug("kscan handler: %d, %d, %d\n", matrix_row, matrix_column, pressed);

    const auto type = key_type(matrix_row, matrix_column);
    if(type == KeyType::ctrl && pressed) {
        const auto key = static_cast<CtrlKey>(matrix_column);
        ctrl_keys_evt.emit(key);
        return;
    }

    uint8_t row, column; get_key_pos(matrix_row, matrix_column, &row, &column);
    const uint8_t index = get_index(row, column);
    const auto state = &key_states[index];

    if(type == KeyType::lower) {
        if(pressed) {
            if(state->timer_state == TimerState::timed_out) {
                l->debug("bottom pressed when already timed out, ignoring\n");
                state->timer_state = TimerState::none;
                return;
            }
            if(state->timer_state == TimerState::running) {
                // cancel job
                scheduler.cancel(index);
                state->timer_state = TimerState::none;
            }
            if(state->velocity != 0) {
                // send it (probably double-clicking lower switch)
                l->debug("sending using existing velocity\n");
            } else if(state->top_ts != 0) {
                // update velocity and send it (regular keypress)
                const uint32_t delay = micros() - state->top_ts;
                state->velocity = static_cast<uint8_t>((1 - std::min(1.0, delay / static_cast<double>(VELOCITY_TIMEOUT))) * 255);
                l->debug("delay was %d, updating velocity to %d and sending press\n", delay, state->velocity);
            } else {
                // error (idk what happened, probably weird switch mechanical stuff or my code is broken)
                state->velocity = VELOCITY_TIMEOUT_VALUE;
                l->debug("unexpected state (bottom pressed before top?), setting to default velocity and sending\n");
                // send it
            }
            send_press(row, column, state);
        } else {
            send_release(row, column, state);
        }
    }
    else /* type == KeyType::upper */ {
        if(pressed) {
            if(state->velocity == 0) {
                l->debug("setting top_ts and starting timeout\n");
                // if velocity is set, the bottom switch would have been pressed before, so already key pressed
                state->top_ts = micros();
                scheduler.schedule(VELOCITY_TIMEOUT, index);
                state->timer_state = TimerState::running;
            } else {
                l->debug("top pressed but velocity is already set, ignoring\n");
            }
        } else {
            // todo: what if only pressed down top, then release before timeout ends? default length?
            state->velocity = 0;
            send_release(row, column, state);

            // temp fix for keys getting stuck on
            if(state->timer_state == TimerState::running) {
                scheduler.cancel(index);
                state->timer_state = TimerState::none;
            }
        }
    }
}