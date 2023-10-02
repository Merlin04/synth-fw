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

//#define I2C_MP_DEBUG

// wire inst corresponding to i2c pins used for multiplexer (16/17)

#define TC9548A_ADDR 0x70

uint8_t cur = 0b00100001;

void tca_write(uint8_t bitmask) {
#ifdef I2C_MP_DEBUG
    Serial.printf("i2c_mp: writing 0x%02x\n", bitmask);
#endif
    MPWire.beginTransmission(TC9548A_ADDR);
    MPWire.write(bitmask);
    auto res = MPWire.endTransmission();
    if(res != 0) {
        Serial.printf("ERR: i2c error in tca_write: %d\n", res);
    }
}

void select_enc(uint8_t enc_index) {
#ifdef I2C_MP_DEBUG
    Serial.printf("i2c_mp: selecting encoder %d\n", enc_index);
#endif
    cur &= 0b11100000;
    cur |= 1 << enc_index;
    tca_write(cur);
}

void select_oled(uint8_t oled_index) {
#ifdef I2C_MP_DEBUG
    Serial.printf("i2c_mp: selecting oled %d\n", oled_index);
#endif
    cur &= 0b10011111;
    cur |= 1 << (oled_index + 5);
    tca_write(cur);
}

void i2c_mp_init() {
#ifdef I2C_MP_DEBUG
    Serial.println("i2c_mp: init");
#endif
    tca_write(cur);
}