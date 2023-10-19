/*
 * Copyright (c) 2020 The ZMK Contributors
 * Copyright (c) Merlin04
 *
 * SPDX-License-Identifier: MIT
 */

#include "kscan_gpio_matrix.hpp"
#include "debounce.hpp"
#include <Arduino.h>
#include "scheduler/scheduler_thread.hpp"

//#define KSCAN_MATRIX_DEBUG

/** Current state of the matrix as a flattened 2D array of length
 * (config->rows * config->cols) */
static struct debounce_state matrix_state[MATRIX_LEN];

kscan_callback_t kscan_callback;
/** Timestamp of the current or scheduled scan, in microseconds. */
uint32_t kscan_scan_time;

static void kscan_matrix_irq_callback_handler();
void kscan_matrix_read(uint8_t poll_counter);

static void kscan_matrix_interrupt_enable() {
#ifdef KSCAN_MATRIX_DEBUG
    Serial.println("kscan_matrix_interrupt_enable");
#endif
    // write all the outputs high so we get interrupts
    // see https://www.infineon.com/dgdl/Infineon-AN2034_PSoC_1_Reading_Matrix_and_Common_Bus_Keypads-ApplicationNotes-v07_00-EN.pdf?fileId=8ac78c8c7cdc391c017d073254b85689
    for(int matrix_output : matrix_outputs) {
        digitalWrite(matrix_output, 1);
    }

    for(int matrix_input : matrix_inputs) {
        attachInterrupt(matrix_input, kscan_matrix_irq_callback_handler, RISING);
    }
}

static void kscan_matrix_interrupt_disable() {
    for(int matrix_input : matrix_inputs) {
        detachInterrupt(matrix_input);
    }
    for(int matrix_output : matrix_outputs) {
        digitalWrite(matrix_output, 0);
    }
}

static void kscan_matrix_irq_callback_handler() {
#ifdef KSCAN_MATRIX_DEBUG
    Serial.println("kscan_matrix_irq_callback_handler");
#endif
    // Disable our interrupts temporarily to avoid re-entry while we scan.
    kscan_matrix_interrupt_disable();

    kscan_scan_time = micros(); // https://www.utopiamechanicus.com/article/handling-arduino-microsecond-overflow/

    kscan_matrix_read(5); // start polling for a bit to try to catch everything
}

static int state_index(const uint8_t input_idx, const uint8_t output_idx) {
    return input_idx * COLS_LEN + output_idx;
}

static int state_index_rc(const uint8_t row, const uint8_t col) {
    return state_index(row, col);
}

SchedulerThread<uint8_t> matrix_scheduler = SchedulerThread<uint8_t>([](uint8_t& i) {
    kscan_matrix_read(i);
});

void kscan_matrix_read(uint8_t poll_counter) {
#ifdef KSCAN_MATRIX_DEBUG
    Serial.println("_kscan_matrix_read");
#endif
    // Scan the matrix.
    delayMicroseconds(5); // hardware is so bad
    for(uint8_t out_idx = 0; out_idx < OUTPUTS_LEN; out_idx++) {
        digitalWrite(matrix_outputs[out_idx], 1);

        for(uint8_t in_idx = 0; in_idx < INPUTS_LEN; in_idx++) {
            if(in_idx == 12 && out_idx < 2) {
                continue;
            }
            const uint8_t index = state_index(in_idx, out_idx);
            const bool active = digitalRead(matrix_inputs[in_idx]) == HIGH; // assume INPUT_PULLDOWN (active high)

#ifdef KSCAN_MATRIX_DEBUG
            if(active) {
                Serial.printf("active: %d, i: %d, j: %d, index: %d\n", active, in_idx, out_idx, index);
            }
#endif

            debounce_update(&matrix_state[index], active, KSCAN_DEBOUNCE_SCAN_PERIOD_MS);
        }

        digitalWrite(matrix_outputs[out_idx], 0);
        delayMicroseconds(KSCAN_COL_DELAY_US); // electron moment, I think this is waiting for the diode to switch?? unclear
    }
#ifdef KSCAN_MATRIX_DEBUG
    Serial.println("_kscan_matrix_read scan done");
#endif

    // Process the new state.
    bool continue_scan = poll_counter > 0; // sometimes an interrupt will be triggered but the switch will jitter a bit and seem like it wasn't pressed
    // but we know it was pressed, so continue even if the debouncer says nothing is active

    for(uint8_t r = 0; r < ROWS_LEN; r++) {
        for(uint8_t c = 0; c < COLS_LEN; c++) {
            const int index = state_index_rc(r, c);
            struct debounce_state* state = &matrix_state[index];

            if(debounce_get_changed(state)) {
                const bool pressed = debounce_is_pressed(state);
#ifdef KSCAN_MATRIX_DEBUG
                Serial.printf("r: %d, c: %d, pressed: %d\n", r, c, pressed);
#endif
                kscan_callback(r, c, pressed);
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
        kscan_scan_time += KSCAN_DEBOUNCE_SCAN_PERIOD_MS * 1000; // microseconds
#ifdef KSCAN_MATRIX_DEBUG
        Serial.printf("continue_scan: %d, data.scan_time: %d, micros(): %d\n", continue_scan, kscan_scan_time, micros());
#endif
        matrix_scheduler.schedule_at(kscan_scan_time, micros(), poll_counter == 0 ? 0 : poll_counter - 1);
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
    kscan_callback = callback;
}

void kscan_matrix_enable() {
#ifdef KSCAN_MATRIX_DEBUG
    Serial.println("kscan_matrix_enable");
#endif
    kscan_scan_time = micros();
    // Read will automatically start interrupts once done.
    kscan_matrix_read(0);
}

void kscan_matrix_init() {
    for(int matrix_input : matrix_inputs) {
        pinMode(matrix_input, INPUT_PULLDOWN);
    }

    for(int matrix_output : matrix_outputs) {
        pinMode(matrix_output, OUTPUT);
        digitalWrite(matrix_output, 0);
    }

    matrix_scheduler.init();
}