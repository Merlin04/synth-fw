//#define I2C_SCAN
//#define MATRIX_TEST

#include <Arduino.h>
#include <TeensyThreads.h>

ThreadWrap(Serial, SerialWrapped)
#define Serial ThreadClone(SerialWrapped)

#include <CrashReport.h>
#include "kscan/kscan_gpio_matrix.hpp"
#include "kscan/velocity.hpp"
#include "ui/ui.hpp"
#include "hardware/i2c_mp.hpp"
#include "hardware/encoder.hpp"
#include "hardware/oled.hpp"
#include "hardware/ctrl_keys.hpp"
#include "hardware/midi.hpp"

#ifdef I2C_SCAN
#include "test/i2c_scan.hpp"
#endif

#ifdef MATRIX_TEST
#include "test/matrix_test.hpp"
#endif
#include "hardware/files.hpp"

// rust ffi
extern "C" int foo();

void setup() {
    for(int i = 0; i < 3; i++) {
        Serial.println("Yeag!");
        delay(1000);
    }

    Serial.println(foo());

    Serial.println("startup");
    Serial.println(CrashReport);

//    threads.stop();

    threads.setDefaultStackSize(8192); // printf causes stack overflow if this isn't set
    threads.setSliceMicros(400); // short slice so it is more responsive - teensy is fast enough :)

#ifdef MATRIX_TEST
    matrix_test();
#endif

    MPWire.begin();
    i2c_mp_init();
#ifdef I2C_SCAN
    i2c_scan_mp(); // use to troubleshoot i2c - takes a bit of time to run
#endif

    encoder_init();

    oled_init();
//    files_init();

    ctrl_keys_evt.add_listener([](const CtrlKey& evt) {
        Serial.printf("ctrl_keys_evt: %d\n", evt);
        return false;
    });

    kscan_matrix_init();
    kscan_matrix_configure(velocity_kscan_handler);
    // velocity_configure([](const uint8_t r, const uint8_t c, const uint8_t velocity, const bool pressed) {
    //     Serial.printf("r: %d, c: %d, velocity: %d, pressed: %d\n", r, c, velocity, pressed);
    // });
    velocity_configure(Midi::velocity_handler);
    velocity_init();
    Midi::init();
    kscan_matrix_enable();

    // ui_init();
}

void loop() {}