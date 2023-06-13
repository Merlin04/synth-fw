#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "ST7789_t3.h" // hardware-specific library
#include <SPI.h>
#include <Fonts/FreeMono9pt7b.h>
#include <TeensyThreads.h>
#include <CrashReport.h>
// #include "kscan/kscan_gpio_matrix.hpp"
#include "kscan/kscan_gpio_direct.hpp"
#include "relay/relay.hpp"

#define SCREEN_X 320
#define SCREEN_Y 240

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

// EXTMEM uint16_t frame_buffer[SCREEN_X * SCREEN_Y];
auto tft = ST7789_t3(TFT_CS, TFT_DC, TFT_RST);

EXTMEM GFXcanvas16 c(SCREEN_X, SCREEN_Y);
// TODO: write custom gfx canvas that uses external memory
uint16_t* frame_buffer = c.getBuffer();

void tftDrawString(const char* text, uint8_t size, uint16_t x, uint16_t y, uint16_t color) {
    Serial.println("tftDrawString");
    Serial.println(text);
    c.setCursor(x, y + 9);
    c.setTextColor(color);
    // c.setTextSize(4);
    c.print(text);
}

Re::DisplayBuffer16 relay_buffer = {
    .pixels = frame_buffer,
    .width = SCREEN_X,
    .height = SCREEN_Y,
    .drawString = tftDrawString
};

constexpr uint8_t frame_rate = 30;

void update_display() {
    threads.addThread([]() {
        // Serial.println("wait update async complete");
        // tft.waitUpdateAsyncComplete();
        // Serial.println("yaeg");
        // tft.updateScreenAsync();
    });
}

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
    kscan_direct_init();
    kscan_direct_configure([](uint8_t _, uint8_t sw, bool pressed) {
        // runs in scan thread
        Serial.printf("sw: %d, pressed: %d, c: %d\n", sw, pressed, count);
        count++;
    });
    kscan_direct_enable();

    // kscan_matrix_init();
    // kscan_matrix_configure([](uint32_t row, uint32_t column, bool pressed) {
    //     // runs in scan thread
    //     Serial.printf("row: %d, column: %d, pressed: %d\n", row, column, pressed);
    // });
    // kscan_matrix_enable();

    /*Serial.println("TFT init");
    // clear frame buffer - inits with random data
    memset(frame_buffer, 0, sizeof(frame_buffer));
    // tft.setFrameBuffer(frame_buffer);
    // tft.useFrameBuffer(true);
    tft.init(240, 320);
    tft.setRotation(1);
    c.setFont(&FreeMono9pt7b);
    // tft.fillScreen(ST77XX_BLACK);
    // delay(1000);
    // tft.fillScreen(ST77XX_BLUE);
    // delay(1000);
    // tft.fillScreen(ST77XX_RED);

    Serial.println("ReLay");

    // auto root = Re::box()
    //     ->width(100)
    //     ->height(400)
    //     ->flexDirection(YGFlexDirectionColumn)
    //     ->bg(255, 0, 255)
    //     ->children();
    
    // Serial.println("ReLay box");
    
    // Re::box()
    //     ->bg(Re::RGB(255, 105, 0))
    //     ->width(100)
    //     ->height(100)
    //     // ->text("Hello, world!");
    //     ->m(5);

    auto time_start = millis();

    auto root = Re::box()
        ->width(320)
        ->height(240)
        ->flexDirection(YGFlexDirectionRow)
        ->justifyContent(YGJustifySpaceBetween)
        ->alignItems(YGAlignStretch)
        // ->bg(Re::RGB(0, 0, 100))
        ->children();
    
    Serial.println("ReLay box");

    Re::box()
        // ->width(100)
        ->height(100)
        ->flex(1)
        ->m(10)
        ->p(10)
        ->border(YGEdgeAll, 5)
        ->bg(Re::RGB(255, 105, 0))
        ->text("Hello, world!");
    
    Re::box()
        ->color(Re::RGB(255, 100, 255))
        // ->bg(0, 0, 100)
        // ->border_color(Re::RGB(0, 0, 100))
        // ->border(YGEdgeAll, 5)
        ->text("testing the text!");
    
    Serial.println("ReLay end");
    
    Re::end();
    Serial.println("ReLay render");
    root->render(&relay_buffer);
    Serial.println("Update display");
    // update_display();
    // tft.updateScreen();
    // debug
    // for(size_t row = 0; row < SCREEN_Y; row++) {
    //     for(size_t col = 0; col < SCREEN_X; col++) {
    //         Serial.printf("%04x ", frame_buffer[row * SCREEN_X + col]);
    //     }
    //     Serial.println();
    // }

    auto time_end = millis();

    tft.writeRect(0, 0, SCREEN_X, SCREEN_Y, frame_buffer);
    auto time_render = millis();

    Serial.printf("Render time: %dms\n", time_end - time_start);
    Serial.printf("Write time: %dms\n", time_render - time_end);*/
}

void loop() {
}