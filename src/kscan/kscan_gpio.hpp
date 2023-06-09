/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

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

// struct kscan_gpio_port_state {
//     const struct device *port;
//     gpio_port_value_t value;
// };

// from Zephyr's source (modified to remove device)
// https://docs.zephyrproject.org/apidoc/2.7.0/group__kscan__interface.html#gab65d45708dba142da2c71aa3debd9480
typedef void(* kscan_callback_t) (uint8_t row, uint8_t column, bool pressed);

// https://zmk.dev/docs/features/debouncing
// instant activate
#define INST_DEBOUNCE_PRESS_MS 0
#define INST_DEBOUNCE_RELEASE_MS 1
#define INST_DEBOUNCE_SCAN_PERIOD_MS 1
#define INST_POLL_PERIOD_MS 10

#define USE_POLLING false
#define USE_INTERRUPTS (!USE_POLLING)

// expand to the code if use_interrupts is true
// (not using an if statement, needs to be entirely preprocessor)
#if USE_POLLING == false
#define COND_INTERRUPTS(code) code
#else
#define COND_INTERRUPTS(code)
#endif

#define COND_POLL_OR_INTERRUPTS(pollcode, intcode) \
    if (USE_POLLING) {                             \
        pollcode;                                  \
    } else {                                       \
        intcode;                                   \
    }
