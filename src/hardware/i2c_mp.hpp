#pragma once

#include <cstdint>
#include <TeensyThreads.h>

void select_enc(uint8_t enc_index);
void select_oled(uint8_t oled_index);
void i2c_mp_init();

inline Threads::Mutex i2c_mutex;