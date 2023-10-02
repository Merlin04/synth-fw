#pragma once
#include "kscan_gpio.hpp"

void kscan_matrix_enable();
void kscan_matrix_init();
void kscan_matrix_configure(kscan_callback_t callback);

// for scheduler
//void kscan_matrix_read();