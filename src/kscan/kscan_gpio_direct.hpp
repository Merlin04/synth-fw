#pragma once
#include "kscan_gpio.hpp"

void kscan_direct_enable();
void kscan_direct_disable();
void kscan_direct_init();
void kscan_direct_configure(const kscan_callback_t callback);

// for scheduler
void kscan_direct_read();