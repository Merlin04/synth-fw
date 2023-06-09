/*
 * Copyright (c) 2021 The ZMK Contributors
 * Copyright (c) Merlin04
 *
 * SPDX-License-Identifier: MIT
 */

// based off of zmk's debounce implementation
// because why rewrite something that already works great
// (hahhahahhahhahhahhahaa that is the motto of this project definitely ,)

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define DEBOUNCE_COUNTER_BITS 14
#define DEBOUNCE_COUNTER_MAX ((1 << DEBOUNCE_COUNTER_BITS) - 1)

struct debounce_state {
    bool pressed : 1;
    bool changed : 1;
    uint16_t counter : DEBOUNCE_COUNTER_BITS;
};

struct debounce_config {
    /** Duration a switch must be pressed to latch as pressed. */
    uint32_t debounce_press_ms;
    /** Duration a switch must be released to latch as released. */
    uint32_t debounce_release_ms;
};

/**
 * Debounces one switch.
 *
 * @param state The state for the switch to debounce.
 * @param active Is the switch currently pressed?
 * @param elapsed_ms Time elapsed since the previous update in milliseconds.
 * @param config Debounce settings.
 */
void debounce_update(struct debounce_state *state, const bool active, const int elapsed_ms,
                     const struct debounce_config *config);

/**
 * @returns whether the switch is either latched as pressed or it is potentially
 * pressed but the debouncer has not yet made a decision. If this returns true,
 * the kscan driver should continue to poll quickly.
 */
bool debounce_is_active(const struct debounce_state *state);

/**
 * @returns whether the switch is latched as pressed.
 */
bool debounce_is_pressed(const struct debounce_state *state);

/**
 * @returns whether the pressed state of the switch changed in the last call to
 * debounce_update.
 */
bool debounce_get_changed(const struct debounce_state *state);