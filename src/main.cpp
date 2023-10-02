//#define I2C_SCAN
//#define MATRIX_TEST

#include <Arduino.h>
#include <Adafruit_GFX.h> // needs to be included for build to work for some reason
#include <TeensyThreads.h>

ThreadWrap(Serial, SerialWrapped)
#define Serial ThreadClone(SerialWrapped)

#include <CrashReport.h>
#include "kscan/kscan_gpio_matrix.hpp"
#include "kscan/velocity.hpp"
#include "scheduler/scheduler_thread.hpp"
#include "ui/ui.hpp"
#include "hardware/i2c_mp.hpp"
#include "hardware/encoder.hpp"
#include "hardware/oled.hpp"
#include "hardware/ctrl_keys.hpp"

#ifdef I2C_SCAN
#include "test/i2c_scan.hpp"
#endif

#ifdef MATRIX_TEST
#include "test/matrix_test.hpp"
#endif
//#include "hardware/files.hpp"

void setup() {
//    Serial.println("Yeag!");
//    delay(5000);

     for(int i = 0; i < 3; i++) {
         Serial.println("Yeag!");
         delay(1000);
     }

    Serial.println("startup");
    Serial.println(CrashReport);

    threads.setDefaultStackSize(2048); // printf causes stack overflow if this isn't set
    threads.setSliceMicros(400); // short slice so it is more responsive - teensy is fast enough :)
    scheduler.init();

#ifdef MATRIX_TEST
    matrix_test();
#endif

    MPWire.begin();
//    MPWire.setClock(100000); // debug
    i2c_mp_init();
#ifdef I2C_SCAN
    i2c_scan_mp(); // use to troubleshoot i2c - takes a bit of time to run
#endif

    encoder_init();

    oled_init();
//    files_init();

    ctrl_keys_evt.add_listener([](CtrlKey& evt) {
        Serial.printf("ctrl_keys_evt: %d\n", evt);
        return false;
    });

    kscan_matrix_init();
    kscan_matrix_configure(velocity_kscan_handler);
    velocity_configure([](uint8_t r, uint8_t c, int8_t velocity, bool pressed) {
        Serial.printf("r: %d, c: %d, velocity: %d, pressed: %d\n", r, c, velocity, pressed);
    });
    kscan_matrix_enable();

//    ui_init();
}

void loop() {}