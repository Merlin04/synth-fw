#include "i2c_mp.hpp"
#include <Wire.h>

//#define ENC_0_CHAN 0
//#define ENC_1_CHAN 1
//#define ENC_2_CHAN 2
//#define ENC_3_CHAN 3
//#define ENC_CTRL_CHAN 4
//
//#define OLED_0_CHAN 5
//#define OLED_1_CHAN 6

#define TC9548A_ADDR 0x70

#define SSD1306_ADDR 0x3C

uint8_t cur = 0b00100001;

void tca_write(uint8_t bitmask) {
    Wire.beginTransmission(TC9548A_ADDR);
    Wire.write(bitmask);
    Wire.endTransmission();
}

void select_enc(uint8_t enc_index) {
    cur &= 0b11100000;
    cur |= 1 << enc_index;
    tca_write(cur);
}

void select_oled(uint8_t oled_index) {
    cur &= 0b10011111;
    cur |= 1 << (oled_index + 5);
    tca_write(cur);
}

void i2c_mp_init() {
    tca_write(cur);
}