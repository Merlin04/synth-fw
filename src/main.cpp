#include <Arduino.h>
#include <Adafruit_GFX.h> // needs to be included for build to work for some reason
#include <TeensyThreads.h>

ThreadWrap(Serial, SerialWrapped)
#define Serial ThreadClone(SerialWrapped)

#include <CrashReport.h>
//#include "kscan/kscan_gpio_matrix.hpp"
//#include "kscan/kscan_gpio_direct.hpp"
//#include "kscan/velocity.hpp"
#include "scheduler/scheduler_thread.hpp"
//#include "ui/ui.hpp"
#include "hardware/i2c_mp.hpp"
#include "hardware/encoder.hpp"
//#include "hardware/oled.hpp"

void setup() {
//    Serial.println("Yeag!");
//    delay(5000);

     for(int i = 0; i < 2; i++) {
         Serial.println("Yeag!");
         delay(1000);
     }

    Serial.println("startup");
    Serial.println(CrashReport);

    threads.setDefaultStackSize(2048); // printf causes stack overflow if this isn't set
    threads.setSliceMicros(400); // short slice so it is more responsive - teensy is fast enough :)
    scheduler.init();

    MPWire.begin();
    i2c_mp_init();
    enc_0.init();
//    encoder_init();
//    oled_init();
//    kscan_matrix_init();
//    kscan_matrix_configure(velocity_kscan_handler);
//    velocity_configure([](uint8_t r, uint8_t c, int8_t velocity, bool pressed) {
//        Serial.printf("r: %d, c: %d, velocity: %d, pressed: %d\n", r, c, velocity, pressed);
//    });
//    kscan_matrix_enable();
//
//    ui_init();
}

void loop() {}