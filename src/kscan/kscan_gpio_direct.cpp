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
#include "kscan_gpio.hpp"
#include "debounce.hpp"
#include <stddef.h>
#include <Arduino.h>
#include <TeensyThreads.h>
#include <EventResponder.h>

#define INST_INPUTS_LEN 6

struct kscan_direct_data {
    struct kscan_gpio_list inputs;
    kscan_callback_t callback;
    /** Timestamp of the current or scheduled scan. */
    int64_t scan_time;
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

static void kscan_direct_read();
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
EventResponder kscan_direct_er_create_scan_thread = EventResponder();

static void kscan_direct_irq_callback_handler() {
    // Disable our interrupts temporarily to avoid re-entry while we scan.
    kscan_direct_interrupt_disable();

    data.scan_time = millis();

    // kscan_direct_scan_thread_id = threads.addThread(kscan_direct_read);

    // Serial.printf("i %d\n", kscan_direct_scan_thread_id);
    kscan_direct_er_create_scan_thread.triggerEvent();
}
#endif

static void kscan_direct_read_continue() {
    data.scan_time += config.debounce_scan_period_ms;

    // run the kscan_direct_read function when millis() == data->scan_time
    kscan_direct_scan_thread_id = threads.addThread([]() {
        threads.delay(data.scan_time - millis());
        kscan_direct_read();
    });
}

static void kscan_direct_read_end() {
    #if USE_INTERRUPTS
    // Return to waiting for an interrupt.
    kscan_direct_interrupt_enable();
    #else
    data.scan_time += config.poll_period_ms;

    // Return to polling slowly.
    kscan_direct_scan_thread_id = threads.addThread([]() {
        threads.delay(data.scan_time - millis());
        kscan_direct_read();
    });
    #endif
}

Threads::Mutex kscan_direct_read_lock; // probably not necessary
static void kscan_direct_read() {
    // Threads::Scope m(kscan_direct_read_lock);
    kscan_direct_read_lock.lock();
    Serial.println("acquired lock");
    // Read the inputs.
    // struct kscan_gpio_port_state state = {0};

    for(uint8_t i = 0; i < data.inputs.len; i++) {
        const struct kscan_gpio* gpio = &data.inputs.gpios[i];

        const bool active = digitalReadFast(gpio->pin) == LOW; // assume INPUT_PULLUP (active low)

        debounce_update(&data.pin_state[gpio->index], active, config.debounce_scan_period_ms, &config.debounce_config);
    }

    // Process the new state.
    bool continue_scan = false;

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
        kscan_direct_read_continue();
    } else {
        // All keys are released. Return to normal.
        kscan_direct_read_end();
    }

    kscan_direct_read_lock.unlock();
    Serial.println("released lock");
}

void kscan_direct_configure(kscan_callback_t callback) {
    data.callback = callback;
}

#if USE_INTERRUPTS
static void kscan_direct_create_scan_thread(EventResponderRef _) {
    Serial.println("when the thread is created :sus:");
    threads.addThread(kscan_direct_read);
    Serial.println("it is")
}
#endif

void kscan_direct_enable() {
    #if USE_INTERRUPTS
    kscan_direct_er_create_scan_thread.attachInterrupt(kscan_direct_create_scan_thread);
    #endif
    data.scan_time = millis();

    // Read will automatically start interrupts/polling once done.
    kscan_direct_read();
}

void kscan_direct_disable() {
    threads.kill(kscan_direct_scan_thread_id);
    #if USE_INTERRUPTS
    kscan_direct_er_create_scan_thread.detach();
    #endif
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