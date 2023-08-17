/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include "kscan_config.hpp"

struct kscan_gpio {
    uint8_t pin;
    /** The index of the GPIO in the devicetree *-gpios array. */
    size_t index;
};

struct kscan_gpio_list {
    struct kscan_gpio *gpios;
    size_t len;
};

// from Zephyr https://docs.zephyrproject.org/apidoc/latest/group__sys-util.html
#define ARRAY_SIZE(array) ((size_t) ((sizeof(array) / sizeof((array)[0]))))

/** Define a kscan_gpio_list from a compile-time GPIO array. */
#define KSCAN_GPIO_LIST(gpio_array)                                                                \
    ((struct kscan_gpio_list){.gpios = gpio_array, .len = ARRAY_SIZE(gpio_array)})

typedef uint32_t gpio_port_value_t;


// from Zephyr's source (modified to remove device)
// https://docs.zephyrproject.org/apidoc/2.7.0/group__kscan__interface.html#gab65d45708dba142da2c71aa3debd9480
typedef void(* kscan_callback_t) (uint8_t row, uint8_t column, bool pressed);

#if INST_DIODE_DIR == KSCAN_ROW2COL
#define COND_DIODE_DIR(row2col_code, col2row_code) row2col_code
#else
#define COND_DIODE_DIR(row2col_code, col2row_code) col2row_code
#endif

#define INST_MATRIX_LEN (INST_ROWS_LEN * INST_COLS_LEN)
#define INST_INPUTS_LEN COND_DIODE_DIR((INST_COLS_LEN), (INST_ROWS_LEN))