//
// Created by me on 8/20/23.
//

#include "oled.hpp"
#include <TeensyThreads.h>
#include "i2c_mp.hpp"

void oled_init() {
    Threads::Scope m(i2c_mutex);

    select_oled(0);
    oled_0.begin(SSD1306_SWITCHCAPVCC, SSD1306_ADDR);
    oled_0.clearDisplay();
    oled_0.display();

    select_oled(1);
    oled_1.begin(SSD1306_SWITCHCAPVCC, SSD1306_ADDR);
    oled_1.clearDisplay();
    oled_1.display();
}