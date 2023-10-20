#include "ui.hpp"
#include <Arduino.h>
#include <Adafruit_GFX.h> // needs to be included for build to work for some reason
#include <ST7789_t3.h>
#include <Fonts/FreeMono9pt7b.h>
#include <TeensyThreads.h>
#include "../relay/relay.hpp"
#include "../hardware/mem_info.hpp"

// display buffer stuff
#define SCREEN_X 320
#define SCREEN_Y 240

#define TFT_CS 10
#define TFT_RST (-1)
#define TFT_DC 9

auto tft = ST7789_t3(TFT_CS, TFT_DC, TFT_RST);

// using extmem makes this way too slow - 67ms vs 0.2ms
// also DMAMEM allows better DMA perf
DMAMEM uint16_t frame_buffer[SCREEN_X * SCREEN_Y];

#define UI_BG RGBA(102, 102, 102, 1)
#define UI_PRIMARY RGBA(255, 91, 15, 1)
#define UI_DARK_PRIMARY RGBA(209, 73, 0, 1) // use for stuff like hover/selections
#define UI_ACCENT RGBA(68, 255, 210) // only use in rare circumstances like selected text
#define UI_LIGHTBG RGBA(151, 151, 155, 1) // use for things like backgrounds of text input
#define UI_TEXT RGBA(255, 255, 255, 1)
#define UI_TEXT_DISABLED RGBA(255, 255, 255, 0.6)
#define UI_BLACK RGBA(23, 3, 18, 1) // use this rarely

constexpr uint8_t frame_rate = 120;

uint32_t last_render_duration = 0;

void ui_render() {
    Serial.println("---------------UI CONSTRUCTION");
    auto root = std::unique_ptr<Re::Box>(Re::box());
    root
        ->width(320)
        ->height(240)
        ->flexDirection(YGFlexDirectionRow)
        ->justifyContent(YGJustifySpaceBetween)
        ->alignItems(YGAlignStretch)
        ->bg(MAKE_RGB(0, 100, 0))
        ->children();

    /*auto buf = new char[100];
    sprintf(buf, "render: %lu us", last_render_duration);
    Re::box()
        ->bg(MAKE_RGB(100, 100, 100))
        ->text(buf)
        ->width(200)
        ->height(50);*/

    Re::box()
        ->flex(1)
        ->bg(MAKE_RGB(100, 0, 20));

    /*Re::box()
//             ->width(100)
            ->height(100)
//            ->flex(1)
            ->m(10)
            ->p(10)
            ->border(YGEdgeAll, 5)
            ->bg(MAKE_RGB(255, 105, 0))
            ->text("Hello, world!");*/

    /*Re::box()
            ->color(MAKE_RGB(255, 100, 255))
            ->bg(MAKE_RGB(0, 0, 100))
            ->border_color(MAKE_RGB(100, 0, 100))
            ->border(YGEdgeAll, 5)
            ->text("testing the\n text!");*/

    Re::end();
    Serial.println("----------------- UI RENDER");
    root->render(&tft);
    Serial.println("------------UI END");
}

void ui_init() {
    memset(frame_buffer, 0, sizeof(frame_buffer));
//    tft.setFrameBuffer(frame_buffer);
//    tft.useFrameBuffer(true);

//    tft.init(240, 320);
//    tft.setRotation(1);
//    tft.setFont(&FreeMono9pt7b);

//    while(true) {
//        Serial.println("-------------UI RENDER");
//        ui_render();
//        delay(100);
//    }

    threads.addThread([]() {
        while(true) {
//            print_mem_info();
//            print_free_itcm();
            auto start = micros();

            ui_render();
            auto render_time = micros();
//            tft.waitUpdateAsyncComplete(); // make sure any previous update is done
//            tft.updateScreenAsync();
            auto update_time = micros();

            last_render_duration = render_time - start;
            Serial.printf("UI: render %d us, update %d us\n", last_render_duration, update_time - render_time);

            auto delay = static_cast<int>(1000000 / frame_rate - (micros() - start));
            if(delay <= 0) {
                Serial.printf("UI WARN: render loop delaying for %d us\n", delay);
            }
            threads.delay_us(delay);
        }
    });
}