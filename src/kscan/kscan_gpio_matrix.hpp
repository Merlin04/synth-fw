#pragma once
#include "kscan_gpio.hpp"

void kscan_matrix_enable();
void kscan_matrix_disable();
void kscan_matrix_init();
void kscan_matrix_configure(const kscan_callback_t callback);