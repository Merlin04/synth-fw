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
    uint16_t pin;
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

struct kscan_gpio_port_state {
    const struct device *port;
    gpio_port_value_t value;
};