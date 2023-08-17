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
#include "scheduler/scheduler_thread.hpp"

struct kscan_direct_data {
    struct kscan_gpio_list inputs;
    kscan_callback_t callback;
    /** Timestamp of the current or scheduled scan, in microseconds. */
    uint32_t scan_time;
    /** Current state of the inputs as an array of length config->inputs.len */
    struct debounce_state* pin_state;
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

void _kscan_direct_read(bool from_irq);
static void kscan_direct_irq_callback_handler();

static void kscan_direct_interrupt_enable() {
    for(uint8_t i = 0; i < data.inputs.len; i++) {
        const struct kscan_gpio* gpio = &data.inputs.gpios[i];
        attachInterrupt(gpio->pin, kscan_direct_irq_callback_handler, RISING);
    }
}

static void kscan_direct_interrupt_disable() {
    for(uint8_t i = 0; i < data.inputs.len; i++) {
        const struct kscan_gpio* gpio = &data.inputs.gpios[i];
        detachInterrupt(gpio->pin);
    }
}

static void kscan_direct_irq_callback_handler() {
    // Disable our interrupts temporarily to avoid re-entry while we scan.
    kscan_direct_interrupt_disable();

    data.scan_time = micros(); // https://www.utopiamechanicus.com/article/handling-arduino-microsecond-overflow/

    _kscan_direct_read(true);
}

void kscan_direct_read() {
    _kscan_direct_read(false);
}
void _kscan_direct_read(bool from_irq) {
    // Read the inputs.
    for(uint8_t i = 0; i < data.inputs.len; i++) {
        const struct kscan_gpio* gpio = &data.inputs.gpios[i];

        const bool active = digitalReadFast(gpio->pin) == LOW; // assume INPUT_PULLUP (active low)

        debounce_update(&data.pin_state[gpio->index], active, INST_DEBOUNCE_SCAN_PERIOD_MS);
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
        data.scan_time += INST_DEBOUNCE_SCAN_PERIOD_MS * 1000; // microseconds
        scheduler.schedule_at(data.scan_time, micros(), -1);
        // kscan_direct_read_timer.trigger(data.scan_time - micros());
    } else {
        // All keys are released. Return to normal.
        // Return to waiting for an interrupt.
        kscan_direct_interrupt_enable();
    }
}

void kscan_direct_configure(kscan_callback_t callback) {
    data.callback = callback;
}

void kscan_direct_enable() {
    data.scan_time = micros();
    // kscan_direct_read_timer.begin(kscan_direct_read);
    // Read will automatically start interrupts once done.
    kscan_direct_read();
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