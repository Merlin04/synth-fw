#pragma once
#include <cstdint>

#define KSCAN_DEBOUNCE_SCAN_PERIOD_MS 1
#define KSCAN_COL_DELAY_US 5

// rows
#define ROWS_LEN 13
#define INPUTS_LEN ROWS_LEN
static int matrix_inputs[] = {35, 36, 37, 38, 39, 40, 41, 14, 15, 25, 24, 12, 7};
// cols
#define COLS_LEN 9
#define OUTPUTS_LEN COLS_LEN
static int matrix_outputs[] = {32, 31, 30, 29, 28, 27, 26, 33, 34};

#define MATRIX_LEN (ROWS_LEN * COLS_LEN)

// from Zephyr's source (modified to remove device)
// https://docs.zephyrproject.org/apidoc/2.7.0/group__kscan__interface.html#gab65d45708dba142da2c71aa3debd9480
typedef void(* kscan_callback_t) (uint8_t row, uint8_t column, bool pressed);

void kscan_matrix_enable();
void kscan_matrix_init();
void kscan_matrix_configure(kscan_callback_t callback);

// for scheduler
//void kscan_matrix_read();