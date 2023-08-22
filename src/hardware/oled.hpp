#pragma once

#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "i2c_mp.hpp"

//class OLED : public Adafruit_SSD1306 {
//
//};

constexpr uint8_t SSD1306_ADDR = 0x3C;

inline auto oled_0 = Adafruit_SSD1306(128, 32, &MPWire, -1);
inline auto oled_1 = Adafruit_SSD1306(128, 32, &MPWire, -1);

void oled_init();