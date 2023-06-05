#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "ST7789_t3.h" // hardware-specific library
#include <SPI.h>
#include <Fonts/FreeSans9pt7b.h>
#include <TeensyThreads.h>
#include "kscan_gpio_matrix.h"

#define SCREEN_X 240
#define SCREEN_Y 320

#define TFT_CS 10
#define TFT_RST -1
#define TFT_DC 9

#define UI_BG RGBA(102, 102, 102, 1)
#define UI_PRIMARY RGBA(255, 91, 15, 1)
#define UI_DARK_PRIMARY RGBA(209, 73, 0, 1) // use for stuff like hover/selections
#define UI_ACCENT RGBA(68, 255, 210) // only use in rare circumstances like selected text
#define UI_LIGHTBG RGBA(151, 151, 155, 1) // use for things like backgrounds of text input
#define UI_TEXT RGBA(255, 255, 255, 1)
#define UI_TEXT_DISABLED RGBA(255, 255, 255, 0.6)
#define UI_BLACK RGBA(23, 3, 18, 1) // use this rarely

EXTMEM uint16_t frame_buffer[SCREEN_X * SCREEN_Y];
auto tft = ST7789_t3(TFT_CS, TFT_DC, TFT_RST);

constexpr uint8_t frame_rate = 30;

void update_display() {
    threads.addThread([]() {
        tft.waitUpdateAsyncComplete();
        tft.updateScreenAsync();
    });
}

void setup() {
    threads.setSliceMicros(400); // short slice so it is more responsive - teensy is fast enough :)
    kscan_matrix_init();
    kscan_matrix_configure([](uint32_t row, uint32_t column, bool pressed) {
        // runs in scan thread
        Serial.printf("row: %d, column: %d, pressed: %d\n", row, column, pressed);
    });
    kscan_matrix_enable();

    tft.setFrameBuffer(frame_buffer);
}

void loop() {
}