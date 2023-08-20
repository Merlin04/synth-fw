#include <Arduino.h>
#include <Adafruit_GFX.h> // needs to be included for build to work for some reason
#include <TeensyThreads.h>
#include <CrashReport.h>
#include "kscan/kscan_gpio_matrix.hpp"
//#include "kscan/kscan_gpio_direct.hpp"
#include "kscan/velocity.hpp"
#include "scheduler/scheduler_thread.hpp"
#include "ui/ui.hpp"
#include "hardware/i2c_mp.hpp"
#include "hardware/encoder.hpp"

uint16_t count = 0;

void setup() {
    Serial.println("Yeag!");
    delay(5000);

    // for(int i = 0; i < 5; i++) {
    //     Serial.println("Yeag!");
    //     delay(1000);
    // }

    Serial.println("startup");
    Serial.println(CrashReport);

    threads.setSliceMicros(400); // short slice so it is more responsive - teensy is fast enough :)
    scheduler.init();
    Wire.begin();
    i2c_mp_init();
    encoder_init();
    kscan_matrix_init();
    // kscan_direct_configure([](uint8_t _, uint8_t sw, bool pressed) {
    //     // runs in scan thread
    //     Serial.printf("sw: %d, pressed: %d, c: %d\n", sw, pressed, count);
    //     count++;
    // });
    kscan_matrix_configure(velocity_kscan_handler);
    velocity_configure([](uint8_t r, uint8_t c, int8_t velocity, bool pressed) {
        Serial.printf("r: %d, c: %d, velocity: %d, pressed: %d\n", r, c, velocity, pressed);
    });
    kscan_matrix_enable();

    ui_init();

    // kscan_matrix_init();
    // kscan_matrix_configure([](uint32_t row, uint32_t column, bool pressed) {
    //     // runs in scan thread
    //     Serial.printf("row: %d, column: %d, pressed: %d\n", row, column, pressed);
    // });
    // kscan_matrix_enable();
}

void loop() {
}