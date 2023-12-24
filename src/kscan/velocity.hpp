#pragma once
#include <cstdint>

typedef void(* velocity_callback_t) (uint8_t row, uint8_t column, uint8_t velocity, bool pressed);

void velocity_kscan_handler(uint8_t matrix_row, uint8_t matrix_column, bool pressed);
void velocity_init();
void velocity_configure(velocity_callback_t callback);