#pragma once
#include <stdint.h>

typedef void(* velocity_callback_t) (uint8_t row, uint8_t column, int8_t velocity, bool pressed);

void test_scheduler();
void velocity_scheduler_cb(int& i);
void velocity_kscan_handler(uint8_t row, uint8_t column, bool pressed);
void velocity_configure(velocity_callback_t callback);