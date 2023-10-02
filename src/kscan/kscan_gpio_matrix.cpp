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

//#define KSCAN_MATRIX_DEBUG

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
    .inputs = KSCAN_GPIO_LIST(COND_DIODE_DIR((kscan_matrix_rows), (kscan_matrix_cols))),
    .outputs = KSCAN_GPIO_LIST(COND_DIODE_DIR((kscan_matrix_cols), (kscan_matrix_rows))),
    .matrix_state = kscan_matrix_state
};

static void kscan_matrix_irq_callback_handler();
void kscan_matrix_read(uint8_t poll_counter);

static void kscan_matrix_interrupt_enable() {
#ifdef KSCAN_MATRIX_DEBUG
    Serial.println("kscan_matrix_interrupt_enable");
#endif
    // write all the outputs high so we get interrupts
    // see https://www.infineon.com/dgdl/Infineon-AN2034_PSoC_1_Reading_Matrix_and_Common_Bus_Keypads-ApplicationNotes-v07_00-EN.pdf?fileId=8ac78c8c7cdc391c017d073254b85689
    for(uint8_t i = 0; i < data.outputs.len; i++) {
        const struct kscan_gpio* gpio = &data.outputs.gpios[i];
        digitalWriteFast(gpio->pin, 1);
    }

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
#ifdef KSCAN_MATRIX_DEBUG
    Serial.println("kscan_matrix_irq_callback_handler");
#endif
    // Disable our interrupts temporarily to avoid re-entry while we scan.
    kscan_matrix_interrupt_disable();

    data.scan_time = micros(); // https://www.utopiamechanicus.com/article/handling-arduino-microsecond-overflow/

    kscan_matrix_read(5); // start polling for a bit to try to catch everything
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

SchedulerThread<uint8_t> matrix_scheduler = SchedulerThread<uint8_t>([](uint8_t& i) {
#ifdef KSCAN_MATRIX_DEBUG
    Serial.printf("matrix_scheduler work: %d\n", i);
#endif
    kscan_matrix_read(i);
});

//void kscan_matrix_read() {
//    _kscan_matrix_read(false);
//}
void kscan_matrix_read(uint8_t poll_counter) {
#ifdef KSCAN_MATRIX_DEBUG
    Serial.println("_kscan_matrix_read");
#endif
    // Scan the matrix.
    for(uint8_t i = 0; i < data.outputs.len; i++) {
#ifdef KSCAN_MATRIX_DEBUG
//        Serial.printf("i: %d\n", i);
#endif
        const struct kscan_gpio* out_gpio = &data.outputs.gpios[i];
        digitalWriteFast(out_gpio->pin, 1);

        for(uint8_t j = 0; j < data.inputs.len; j++) {
#ifdef KSCAN_MATRIX_DEBUG
//            Serial.printf("j: %d\n", j);
#endif
            const struct kscan_gpio* in_gpio = &data.inputs.gpios[j];

            const uint8_t index = state_index_io(in_gpio->index, out_gpio->index);
            const bool active = digitalReadFast(in_gpio->pin) == HIGH; // assume INPUT_PULLDOWN (active high)
#ifdef KSCAN_MATRIX_DEBUG
            if(active) {
                Serial.printf("active: %d, i: %d, j: %d, index: %d\n", active, i, j, index);
            }
#endif

            debounce_update(&data.matrix_state[index], active, INST_DEBOUNCE_SCAN_PERIOD_MS);
        }

        digitalWriteFast(out_gpio->pin, 0);
        delayMicroseconds(5); // electron moment, I think this is waiting for the diode to switch?? unclear
    }
#ifdef KSCAN_MATRIX_DEBUG
    Serial.println("_kscan_matrix_read scan done");
#endif

    // Process the new state.
    bool continue_scan = poll_counter > 0; // sometimes an interrupt will be triggered but the switch will jitter a bit and seem like it wasn't pressed
    // but we know it was pressed, so continue even if the debouncer says nothing is active

    for(uint8_t r = 0; r < INST_ROWS_LEN; r++) {
        for(uint8_t c = 0; c < INST_COLS_LEN; c++) {
            const int index = state_index_rc(r, c);
            struct debounce_state* state = &data.matrix_state[index];

            if(debounce_get_changed(state)) {
                const bool pressed = debounce_is_pressed(state);
#ifdef KSCAN_MATRIX_DEBUG
                Serial.printf("r: %d, c: %d, pressed: %d\n", r, c, pressed);
#endif
                data.callback(r, c, pressed);
            }

            continue_scan = continue_scan || debounce_is_active(state);
        }
    }

#ifdef KSCAN_MATRIX_DEBUG
    Serial.println("_kscan_matrix_read process done");
#endif

    if(continue_scan) {
        // At least one key is pressed or the debouncer has not yet decided if
        // it is pressed. Poll quickly until everything is released.
        data.scan_time += INST_DEBOUNCE_SCAN_PERIOD_MS * 1000; // microseconds
#ifdef KSCAN_MATRIX_DEBUG
        Serial.printf("continue_scan: %d, data.scan_time: %d, micros(): %d\n", continue_scan, data.scan_time, micros());
#endif
        matrix_scheduler.schedule_at(data.scan_time, micros(), poll_counter == 0 ? 0 : poll_counter - 1);
    } else {
#ifdef KSCAN_MATRIX_DEBUG
        Serial.println("not continuing scan");
#endif
        // All keys are released. Return to normal.
        // Return to waiting for an interrupt.
        kscan_matrix_interrupt_enable();
    }
}

void kscan_matrix_configure(kscan_callback_t callback) {
    data.callback = callback;
}

void kscan_matrix_enable() {
#ifdef KSCAN_MATRIX_DEBUG
    Serial.println("kscan_matrix_enable");
#endif
    data.scan_time = micros();
    // Read will automatically start interrupts once done.
    kscan_matrix_read(0);
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

    matrix_scheduler.init();
}