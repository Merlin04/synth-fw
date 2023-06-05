#pragma once
#include <stdint.h>

// from Zephyr's source (modified to remove device)
// https://docs.zephyrproject.org/apidoc/2.7.0/group__kscan__interface.html#gab65d45708dba142da2c71aa3debd9480
typedef void(* kscan_callback_t) (uint32_t row, uint32_t column, bool pressed);

static void kscan_matrix_enable();
static void kscan_matrix_disable();
static void kscan_matrix_init();
static void kscan_matrix_configure(const kscan_callback_t callback);