#pragma once
#include <cstdint>
#include <Wire.h>
#include "../util/thread.hpp"
#include "i2c_mp.hpp"

constexpr uint8_t AS5600_ADDR = 0x36;

constexpr uint8_t AS5600_STATUS = 0x0B;
constexpr uint8_t AS5600_MAGNET_HIGH = 0x08;
constexpr uint8_t AS5600_MAGNET_LOW = 0x10;
constexpr uint8_t AS5600_MAGNET_DETECT = 0x20;
constexpr uint8_t AS5600_OUT_ANGLE = 0x0E;

class Encoder : public Thread<Encoder> {
private:
    uint8_t index;
    uint16_t steps_per_inc = 128;
    int16_t step_count = 0;
    uint16_t prev_angle = 0;

    using EncoderCallback = void (*)(int16_t incs);
    EncoderCallback callback;

    [[nodiscard]] uint8_t read_register(uint8_t reg) const {
        Threads::Scope m(i2c_mutex);

        select_enc(index);
        Wire.beginTransmission(AS5600_ADDR);
        Wire.write(reg);
        Wire.endTransmission();

        Wire.requestFrom(AS5600_ADDR, (uint8_t)1);
        return Wire.read();
    }

    [[nodiscard]] uint16_t read_register_16(uint8_t reg) const {
        Threads::Scope m(i2c_mutex);

        select_enc(index);
        Wire.beginTransmission(AS5600_ADDR);
        Wire.write(reg);
        Wire.endTransmission();

        Wire.requestFrom(AS5600_ADDR, (uint8_t)2);
        uint16_t val = Wire.read();
        val <<= 8;
        val |= Wire.read();
        return val;
    }

    // read a value from 0 to 4095 representing the current angle of the encoder
    [[nodiscard]] uint16_t read_angle() const {
        return read_register_16(AS5600_OUT_ANGLE) & 0x0FFF;
    }

    [[noreturn]] void thread_fn() {
        while(true) {
            auto angle = read_angle();
            int16_t diff = angle - prev_angle;
            if(diff > 2048) {
                diff -= 4096;
            } else if(diff < -2048) {
                diff += 4096;
            }
            step_count += diff;
            prev_angle = angle;
            int16_t incs = step_count / steps_per_inc;
            if(incs != 0) {
                step_count %= steps_per_inc;
                callback(incs);
            }

            threads.delay(2); // ??
        }
    }
    friend class Thread<Encoder>;

public:
    Encoder(uint8_t index, EncoderCallback cb) : index(index), callback(cb) {}
    Encoder(uint8_t index, uint8_t incs_per_rev, EncoderCallback cb) : index(index), callback(cb) {
        set_steps_per_inc(incs_per_rev);
    }

    void set_steps_per_inc(uint16_t incs_per_rev) {
        // there are 4096 steps per rev
        steps_per_inc = 4096 / incs_per_rev;
        step_count = 0;
    }

    bool init() {
        thread_init();

        auto status = read_register(AS5600_STATUS);
        if((status & AS5600_MAGNET_DETECT) > 1) {
            Serial.printf("ERR: no magnet detected for encoder %d\n", index);
            return false;
        }
        if((status & AS5600_MAGNET_HIGH) > 1) {
            Serial.printf("ERR: magnet too strong for encoder %d\n", index);
            return false;
        }
        if((status & AS5600_MAGNET_LOW) > 1) {
            Serial.printf("ERR: magnet too weak for encoder %d\n", index);
            return false;
        }

        prev_angle = read_angle();

        return true;
    }
};

inline auto enc_0 = Encoder(0, [](int16_t incs) {
    Serial.printf("enc 0: %d\n", incs);
});
inline auto enc_1 = Encoder(1, [](int16_t incs) {
    Serial.printf("enc 1: %d\n", incs);
});
inline auto enc_2 = Encoder(2, [](int16_t incs) {
    Serial.printf("enc 2: %d\n", incs);
});
inline auto enc_3 = Encoder(3, [](int16_t incs) {
    Serial.printf("enc 3: %d\n", incs);
});
inline auto enc_ctrl = Encoder(4, 24, [](int16_t incs) {
    Serial.printf("enc ctrl: %d\n", incs);
});

void encoder_init();