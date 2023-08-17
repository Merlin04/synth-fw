/*
 * Copyright (c) 2020 The ZMK Contributors
 * Copyright (c) Merlin04
 * 
 * This direct driver is used for testing. The actual device uses a keymatrix
 * (see `kscan_gpio_matrix.cpp`).
 *
 * SPDX-License-Identifier: MIT
 */

#include "kscan_gpio_direct.hpp"
#include "kscan_config.hpp"
#include "kscan_gpio.hpp"
#include "debounce.hpp"
#include <Arduino.h>
#include <TeensyThreads.h>
#include "scheduler/scheduler_thread.hpp"

struct kscan_direct_data {
    struct kscan_gpio_list inputs;
    kscan_callback_t callback;
    /** Timestamp of the current or scheduled scan, in microseconds. */
    uint32_t scan_time;
    /** Current state of the inputs as an array of length config->inputs.len */
    struct debounce_state* pin_state;
};

struct kscan_direct_config {
    struct debounce_config debounce_config;
    int32_t debounce_scan_period_ms;
    int32_t poll_period_ms;
};

static struct kscan_gpio kscan_direct_inputs[] = {
    { .pin = 41, .index = 0},
    { .pin = 40, .index = 1},
    { .pin = 39, .index = 2},
    { .pin = 38, .index = 3},
    { .pin = 37, .index = 4},
    { .pin = 36, .index = 5}
};

static struct debounce_state kscan_direct_state[INST_INPUTS_LEN];

volatile static struct kscan_direct_data data = {
    .inputs = KSCAN_GPIO_LIST(kscan_direct_inputs),
    .pin_state = kscan_direct_state,
};

static struct kscan_direct_config config = {
    .debounce_config = {
        .debounce_press_ms = INST_DEBOUNCE_PRESS_MS,
        .debounce_release_ms = INST_DEBOUNCE_RELEASE_MS
    },
    .debounce_scan_period_ms = INST_DEBOUNCE_SCAN_PERIOD_MS,
    .poll_period_ms = INST_POLL_PERIOD_MS
};

volatile int kscan_direct_scan_thread_id;

void _kscan_direct_read(bool from_irq);
#if USE_INTERRUPTS
static void kscan_direct_irq_callback_handler();
#endif

#if USE_INTERRUPTS
static void kscan_direct_interrupt_enable() {
    for(uint8_t i = 0; i < data.inputs.len; i++) {
        const struct kscan_gpio* gpio = &data.inputs.gpios[i];
        attachInterrupt(gpio->pin, kscan_direct_irq_callback_handler, RISING);
    }
}
#endif

#if USE_INTERRUPTS
static void kscan_direct_interrupt_disable() {
    for(uint8_t i = 0; i < data.inputs.len; i++) {
        const struct kscan_gpio* gpio = &data.inputs.gpios[i];
        detachInterrupt(gpio->pin);
    }
}
#endif

#if USE_INTERRUPTS
static void kscan_direct_irq_callback_handler() {
    // Disable our interrupts temporarily to avoid re-entry while we scan.
    kscan_direct_interrupt_disable();

    data.scan_time = micros(); // https://www.utopiamechanicus.com/article/handling-arduino-microsecond-overflow/

    _kscan_direct_read(true);
}
#endif

void kscan_direct_read() {
    _kscan_direct_read(false);
}
void _kscan_direct_read(bool from_irq) {
    // Read the inputs.
#if USE_POLLING
KSCAN_DIRECT_READ_START:
#endif
    for(uint8_t i = 0; i < data.inputs.len; i++) {
        const struct kscan_gpio* gpio = &data.inputs.gpios[i];

        const bool active = digitalReadFast(gpio->pin) == LOW; // assume INPUT_PULLUP (active low)

        debounce_update(&data.pin_state[gpio->index], active, config.debounce_scan_period_ms, &config.debounce_config);
    }

    // Process the new state.
    bool continue_scan = from_irq; // sometimes an interrupt will be triggered but the switch will jitter a bit and seem like it wasn't pressed
    // but we know it was pressed, so continue even if the debouncer says nothing is active
    // TODO: if this doesn't catch all of this happening, add a counter that retries some n times

    for(uint8_t i = 0; i < data.inputs.len; i++) {
        const struct kscan_gpio* gpio = &data.inputs.gpios[i];
        struct debounce_state* state = &data.pin_state[gpio->index];

        if(debounce_get_changed(state)) {
            const bool pressed = debounce_is_pressed(state);
            data.callback(0, gpio->index, pressed);
        }

        continue_scan = continue_scan || debounce_is_active(state);
    }

    if(continue_scan) {
        // At least one key is pressed or the debouncer has not yet decided if
        // it is pressed. Poll quickly until everything is released.
        data.scan_time += config.debounce_scan_period_ms * 1000; // microseconds
        #if USE_INTERRUPTS
        scheduler.schedule_at(data.scan_time, micros(), -1);
        // kscan_direct_read_timer.trigger(data.scan_time - micros());
        #else
        threads.delay_us(data.scan_time - micros());
        goto KSCAN_DIRECT_READ_START;
        #endif
    } else {
        // All keys are released. Return to normal.
        #if USE_INTERRUPTS
        // Return to waiting for an interrupt.
        kscan_direct_interrupt_enable();
        #else
        data.scan_time += config.poll_period_ms * 1000;

        // Return to polling slowly.
        threads.delay_us(data.scan_time - micros());
        goto KSCAN_DIRECT_READ_START;
        #endif
    }
}

void kscan_direct_configure(kscan_callback_t callback) {
    data.callback = callback;
}

void kscan_direct_enable() {
    data.scan_time = micros();
    #if USE_INTERRUPTS
    // kscan_direct_read_timer.begin(kscan_direct_read);
    // Read will automatically start interrupts/polling once done.
    kscan_direct_read();
    #else
    kscan_direct_scan_thread_id = threads.addThread(kscan_direct_read);
    #endif
}

void kscan_direct_disable() {
    threads.kill(kscan_direct_scan_thread_id);
}

static void kscan_direct_init_input_inst(const struct kscan_gpio* gpio, const int index) {
    pinMode(gpio->pin, INPUT_PULLUP);
}

static void kscan_direct_init_inputs() {
    for(uint8_t i = 0; i < data.inputs.len; i++) {
        const struct kscan_gpio* gpio = &data.inputs.gpios[i];
        kscan_direct_init_input_inst(gpio, i);
    }
}

void kscan_direct_init() {
    kscan_direct_init_inputs();
}