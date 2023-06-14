/*
 * Copyright (c) 2020-2021 The ZMK Contributors
 * Copyright (c) Merlin04
 *
 * SPDX-License-Identifier: MIT
 */

#include "kscan_gpio_matrix.hpp"
#include "kscan_config.hpp"
#include "kscan_gpio.hpp"
#include "debounce.hpp"
#include <stddef.h>
#include <Arduino.h>
#include <TeensyThreads.h>

static struct kscan_gpio kscan_matrix_rows[] = {
    // TODO actual values
    { .pin = 5, .index = 0},
    { .pin = 6, .index = 1},
    { .pin = 7, .index = 2},
    { .pin = 8, .index = 3},
    { .pin = 9, .index = 4},
    { .pin = 10, .index = 5},
    { .pin = 11, .index = 6}
};

static struct kscan_gpio kscan_matrix_cols[] = {
    // TODO actual values
    { .pin = 12, .index = 0 },
    { .pin = 13, .index = 1 },
    { .pin = 14, .index = 2 },
    { .pin = 15, .index = 3 },
    { .pin = 16, .index = 4 },
    { .pin = 17, .index = 5 },
    { .pin = 18, .index = 6 },
    { .pin = 19, .index = 7 },
    { .pin = 20, .index = 8 }
};

enum kscan_diode_direction {
    KSCAN_ROW2COL,
    KSCAN_COL2ROW,
};

struct kscan_matrix_data {
    struct kscan_gpio_list inputs;
    kscan_callback_t callback;
    #if USE_INTERRUPTS
    /** Array of length config->inputs.len */
    // struct kscan_matrix_irq_callback* irqs;
    #endif
    /** Timestamp of the current or scheduled scan. */
    int64_t scan_time;
    /**
     * Current state of the matrix as a flattened 2D array of length
     * (config->rows * config->cols)
     */
    struct debounce_state* matrix_state;
};

struct kscan_matrix_config {
    struct kscan_gpio_list outputs;
    struct debounce_config debounce_config;
    size_t rows;
    size_t cols;
    int32_t debounce_scan_period_ms;
    int32_t poll_period_ms;
    enum kscan_diode_direction diode_direction;
};

int kscan_matrix_scan_thread_id;

static struct debounce_state kscan_matrix_state[INST_MATRIX_LEN];

// COND_INTERRUPTS(static struct kscan_matrix_irq_callback kscan_matrix_irqs[INST_INPUTS_LEN];)

static struct kscan_matrix_data data = {
    .inputs = KSCAN_GPIO_LIST(COND_DIODE_DIR((kscan_matrix_cols), (kscan_matrix_rows))),
    .matrix_state = kscan_matrix_state,
    // COND_INTERRUPTS(.irqs = kscan_matrix_irqs),
};

static struct kscan_matrix_config config = {
    .outputs = KSCAN_GPIO_LIST(COND_DIODE_DIR((kscan_matrix_rows), (kscan_matrix_cols))),
    .debounce_config = {
        .debounce_press_ms = INST_DEBOUNCE_PRESS_MS,
        .debounce_release_ms = INST_DEBOUNCE_RELEASE_MS
    },
    .rows = ARRAY_SIZE(kscan_matrix_rows),
    .cols = ARRAY_SIZE(kscan_matrix_cols),
    .debounce_scan_period_ms = INST_DEBOUNCE_SCAN_PERIOD_MS,
    .poll_period_ms = INST_POLL_PERIOD_MS,
    .diode_direction = INST_DIODE_DIR
};

static void kscan_matrix_read();
#if USE_INTERRUPTS
static void kscan_matrix_irq_callback_handler();
#endif

static int state_index_rc(const int row, const int col) {
    return (col * config.rows) + row;
}

/**
 * Get the index into a matrix state array from input/output pin indices.
 */
static int state_index_io(const int input_idx, const int output_idx) {
    return (config.diode_direction == KSCAN_ROW2COL)
                ? state_index_rc(output_idx, input_idx)
                : state_index_rc(input_idx, output_idx);
}

static void kscan_matrix_set_all_outputs(const uint8_t value) {
    for(int i = 0; i < config.outputs.len; i++) {
        digitalWriteFast(config.outputs.gpios[i].pin, value);
    }
}

#if USE_INTERRUPTS
static void kscan_matrix_interrupt_enable() {
    for(uint8_t i = 0; i < data.inputs.len; i++) {
        const struct kscan_gpio* gpio = &data.inputs.gpios[i];
        attachInterrupt(gpio->pin, kscan_matrix_irq_callback_handler, RISING);
    }

    // While interrupts are enabled, set all outputs active so a pressed key
    // will trigger an interrupt.
    kscan_matrix_set_all_outputs(1);
}
#endif

#if USE_INTERRUPTS
static void kscan_matrix_interrupt_disable() {
    for(uint8_t i = 0; i < data.inputs.len; i++) {
        const struct kscan_gpio* gpio = &data.inputs.gpios[i];
        detachInterrupt(gpio->pin);
    }

    // While interrupts are disabled, set all outputs inactive so
    // kscan_matrix_read() can scan them one by one.
    kscan_matrix_set_all_outputs(0);
}
#endif

#if USE_INTERRUPTS
static void kscan_matrix_irq_callback_handler() {
    // Disable our interrupts temporarily to avoid re-entry while we scan.
    kscan_matrix_interrupt_disable();

    data.scan_time = millis();

    kscan_matrix_scan_thread_id = threads.addThread(kscan_matrix_read);
}
#endif

static void kscan_matrix_read_continue() {
    data.scan_time += config.debounce_scan_period_ms;

    // run the kscan_matrix_read function when millis() == data.scan_time
    kscan_matrix_scan_thread_id = threads.addThread([]() {
        threads.delay(data.scan_time - millis());
        kscan_matrix_read();
    });
}

static void kscan_matrix_read_end() {
    #if USE_INTERRUPTS
    // Return to waiting for an interrupt.
    kscan_matrix_interrupt_enable();
    #else
    data.scan_time += config.poll_period_ms;
    kscan_matrix_scan_thread_id = threads.addThread([]() {
        threads.delay(data.scan_time - millis());
        kscan_matrix_read();
    });
    #endif
}

Threads::Mutex kscan_matrix_read_lock;
static void kscan_matrix_read() {
    Threads::Scope m(kscan_matrix_read_lock); // only run one read thread at a time - there shouldn't be multiple scheduled in the first place but this prevents that from doing anything too bad
    // Scan the matrix.
    for(int i = 0; i < config.outputs.len; i++) {
        const struct kscan_gpio* out_gpio = &config.outputs.gpios[i];
        digitalWriteFast(out_gpio->pin, 1);

        #if CONFIG_ZMK_KSCAN_MATRIX_WAIT_BEFORE_INPUTS > 0
        threads.delay(CONFIG_ZMK_KSCAN_MATRIX_WAIT_BEFORE_INPUTS);
        #endif

        // struct kscan_gpio_port_state state = {0};

        for(int j = 0; j < data.inputs.len; j++) {
            const struct kscan_gpio* in_gpio = &data.inputs.gpios[j];

            const int index = state_index_io(in_gpio->index, out_gpio->index);
            const int active = digitalReadFast(in_gpio->pin);

            debounce_update(&data.matrix_state[index], active, config.debounce_scan_period_ms, &config.debounce_config);
        }

        digitalWriteFast(out_gpio->pin, 0);

        #if CONFIG_ZMK_KSCAN_MATRIX_WAIT_BETWEEN_OUTPUTS > 0
        threads.delay(CONFIG_ZMK_KSCAN_MATRIX_WAIT_BETWEEN_OUTPUTS);
        #endif
    }

    // Process the new state.
    bool continue_scan = false;

    for(int r = 0; r < config.rows; r++) {
        for(int c = 0; c < config.cols; c++) {
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
        kscan_matrix_read_continue();
    } else {
        // All keys are released. Return to normal.
        kscan_matrix_read_end();
    }
}

// omitted definition of kscan_matrix_work_handler - just calls kscan_matrix_read

void kscan_matrix_configure(const kscan_callback_t callback) {
    data.callback = callback;
}

void kscan_matrix_enable() {
    data.scan_time = millis();

    // Read will automatically start interrupts/polling once done.
    kscan_matrix_scan_thread_id = threads.addThread(kscan_matrix_read);
}

void kscan_matrix_disable() {
    threads.kill(kscan_matrix_scan_thread_id);

    #if USE_INTERRUPTS
    kscan_matrix_interrupt_disable();
    #endif
}

static void kscan_matrix_init_input_inst(const struct kscan_gpio* gpio) {
    pinMode(gpio->pin, INPUT);
}

static void kscan_matrix_init_inputs() {
    for(int i = 0; i < data.inputs.len; i++) {
        const struct kscan_gpio* gpio = &data.inputs.gpios[i];
        kscan_matrix_init_input_inst(gpio);
    }
}

static void kscan_matrix_init_output_inst(const struct kscan_gpio* gpio) {
    pinMode(gpio->pin, OUTPUT);
}

static void kscan_matrix_init_outputs() {
    for(int i = 0; i < config.outputs.len; i++) {
        const struct kscan_gpio* gpio = &config.outputs.gpios[i];
        kscan_matrix_init_output_inst(gpio);
    }
}

void kscan_matrix_init() {
    kscan_matrix_init_inputs();
    kscan_matrix_init_outputs();
    kscan_matrix_set_all_outputs(0);

    // thread setup stuff would go here, if I had any
}