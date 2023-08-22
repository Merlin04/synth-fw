/*
 * Copyright (c) 2020 The ZMK Contributors
 * Copyright (c) Merlin04
 *
 * SPDX-License-Identifier: MIT
 */

#include "kscan_gpio_matrix.hpp"
#include "kscan_config.hpp"
#include "kscan_gpio.hpp"
#include "debounce.hpp"
#include <Arduino.h>
#include "scheduler/scheduler_thread.hpp"

struct kscan_matrix_data {
    struct kscan_gpio_list inputs;
    struct kscan_gpio_list outputs;
    kscan_callback_t callback;
    /** Timestamp of the current or scheduled scan, in microseconds. */
    uint32_t scan_time;
    /** Current state of the matrix as a flattened 2D array of length
     * (config->rows * config->cols) */
    struct debounce_state* matrix_state;
};

static struct kscan_gpio kscan_matrix_rows[] = {
        { .pin = 35, .index = 0 },
        { .pin = 36, .index = 1 },
        { .pin = 37, .index = 2 },
        { .pin = 38, .index = 3 },
        { .pin = 39, .index = 4 },
        { .pin = 40, .index = 5 },
        { .pin = 41, .index = 6 },
        { .pin = 14, .index = 7 },
        { .pin = 15, .index = 8 },
        { .pin = 25, .index = 9 },
        { .pin = 24, .index = 10 },
        { .pin = 12, .index = 11 },
        { .pin = 7, .index = 12 }
};

static struct kscan_gpio kscan_matrix_cols[] = {
        { .pin = 32, .index = 0 },
        { .pin = 31, .index = 1 },
        { .pin = 30, .index = 2 },
        { .pin = 29, .index = 3 },
        { .pin = 28, .index = 4 },
        { .pin = 27, .index = 5 },
        { .pin = 26, .index = 6 },
        { .pin = 33, .index = 7 },
        { .pin = 34, .index = 8 }
};

static struct debounce_state kscan_matrix_state[INST_MATRIX_LEN];

volatile static struct kscan_matrix_data data = {
    .inputs = KSCAN_GPIO_LIST(COND_DIODE_DIR((kscan_matrix_cols), (kscan_matrix_rows))),
    .outputs = KSCAN_GPIO_LIST(COND_DIODE_DIR((kscan_matrix_rows), (kscan_matrix_cols))),
    .matrix_state = kscan_matrix_state
};

void _kscan_matrix_read(bool from_irq);
static void kscan_matrix_irq_callback_handler();

static void kscan_matrix_interrupt_enable() {
    for(uint8_t i = 0; i < data.inputs.len; i++) {
        const struct kscan_gpio* gpio = &data.inputs.gpios[i];
        attachInterrupt(gpio->pin, kscan_matrix_irq_callback_handler, RISING);
    }
}

static void kscan_matrix_interrupt_disable() {
    for(uint8_t i = 0; i < data.inputs.len; i++) {
        const struct kscan_gpio* gpio = &data.inputs.gpios[i];
        detachInterrupt(gpio->pin);
    }
}

static void kscan_matrix_irq_callback_handler() {
    // Disable our interrupts temporarily to avoid re-entry while we scan.
    kscan_matrix_interrupt_disable();

    data.scan_time = micros(); // https://www.utopiamechanicus.com/article/handling-arduino-microsecond-overflow/

    _kscan_matrix_read(true);
}

static int state_index_rc(const uint8_t row, const uint8_t col) {
    return (col * INST_ROWS_LEN) + row;
}

/**
 * Get the index into a matrix state array from input/output pin indices.
 */
static int state_index_io(const uint8_t input_idx, const uint8_t output_idx) {
    return COND_DIODE_DIR(state_index_rc(output_idx, input_idx), state_index_rc(input_idx, output_idx));
}

void kscan_matrix_read() {
    _kscan_matrix_read(false);
}
void _kscan_matrix_read(bool from_irq) {
    // Scan the matrix.
    for(uint8_t i = 0; i < data.outputs.len; i++) {
        const struct kscan_gpio* out_gpio = &data.outputs.gpios[i];
        digitalWriteFast(out_gpio->pin, 1);

        for(uint8_t j = 0; j < data.inputs.len; i++) {
            const struct kscan_gpio* in_gpio = &data.inputs.gpios[j];

            const uint8_t index = state_index_io(in_gpio->index, out_gpio->index);
            const bool active = digitalReadFast(in_gpio->pin) == HIGH; // assume INPUT_PULLDOWN (active high)

            debounce_update(&data.matrix_state[index], active, INST_DEBOUNCE_SCAN_PERIOD_MS);
        }

        digitalWriteFast(out_gpio->pin, 0);
    }

    // Process the new state.
    bool continue_scan = from_irq; // sometimes an interrupt will be triggered but the switch will jitter a bit and seem like it wasn't pressed
    // but we know it was pressed, so continue even if the debouncer says nothing is active
    // TODO: if this doesn't catch all of this happening, add a counter that retries some n times

    for(uint8_t r = 0; r < INST_ROWS_LEN; r++) {
        for(uint8_t c = 0; c < INST_COLS_LEN; c++) {
            const int index = state_index_rc(r, c);
            struct debounce_state* state = &data.matrix_state[index];

            if(debounce_get_changed(state)) {
                const bool pressed = debounce_is_pressed(state);
                data.callback(r, c, pressed);
            }

            continue_scan = continue_scan || debounce_is_active(state);
        }
    }

    if(continue_scan) {
        // At least one key is pressed or the debouncer has not yet decided if
        // it is pressed. Poll quickly until everything is released.
        data.scan_time += INST_DEBOUNCE_SCAN_PERIOD_MS * 1000; // microseconds
        scheduler.schedule_at(data.scan_time, micros(), -1);
    } else {
        // All keys are released. Return to normal.
        // Return to waiting for an interrupt.
        kscan_matrix_interrupt_enable();
    }
}

void kscan_matrix_configure(kscan_callback_t callback) {
    data.callback = callback;
}

void kscan_matrix_enable() {
    data.scan_time = micros();
    // Read will automatically start interrupts once done.
    kscan_matrix_read();
}

void kscan_matrix_init() {
    for(uint8_t i = 0; i < data.inputs.len; i++) {
        const struct kscan_gpio* gpio = &data.inputs.gpios[i];
        pinMode(gpio->pin, INPUT_PULLDOWN);
    }

    for(uint8_t i = 0; i < data.outputs.len; i++) {
        const struct kscan_gpio* gpio = &data.outputs.gpios[i];
        pinMode(gpio->pin, OUTPUT);
        digitalWriteFast(gpio->pin, 0);
    }
}