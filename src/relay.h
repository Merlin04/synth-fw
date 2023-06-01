/*
 * ReLay: _Re_active _Lay_out
 * (header file - see the cpp file for more details)
 * This file primarily contains struct definitions.
 */

#pragma once
#include <stdint.h>
#include <stdlib.h>

namespace Re {
    // simple color struct
    struct RGBA {
        uint8_t r = 0, g = 0, b = 0;
        float a = 1.0f;
        
        uint16_t to565() {
            return (r & 0xF8) << 8 | (g & 0xFC) << 3 | (b & 0xF8) >> 3;
        }

        // returns the rendered color with a background of `color`, applying the alpha value of this color
        RGBA applyAlpha(RGBA* color) const {
            return {
                (uint8_t) (r * a + color->r * (1.0f - a)),
                (uint8_t) (g * a + color->g * (1.0f - a)),
                (uint8_t) (b * a + color->b * (1.0f - a)),
                1.0f
            };
        }

        static RGBA from565(uint16_t color) {
            return {
                (uint8_t) ((color & 0xF800) >> 8),
                (uint8_t) ((color & 0x07E0) >> 3),
                (uint8_t) ((color & 0x001F) << 3),
                1.0f
            };
        }

        RGBA applyAlpha(uint16_t color) const {
            return applyAlpha(&from565(color));
        }
    };

    // size value, holds a value and a type (px or %)
    enum SizeType {
        PX,
        PERCENT
    };
    struct SizeValue {
        uint16_t value = 0;
        SizeType type = PX;

        /*uint16_t resolve(uint16_t relativeTo) const {
            if(type == PX) return value;
            return relativeTo * value / 100;
        }*/

        void resolve(uint16_t relativeTo) {
            if(type == PX) return;
            value = relativeTo * value / 100;
            type = PX;
        }
    };

    struct Text {
        const char* text = nullptr;
        size_t length = 0;
    };

    struct Pixels {
        uint16_t* pixels = nullptr;
        uint16_t width = 0, height = 0;
    };

    // this is the primary primitive that ReLay can render. it can be styled through a variety of attributes
    // and can contain children (either a single text node, a single pixels node, or multiple other `BaseBox`es).
    struct Box {
        SizeValue x, y, w, h,     // position
                  pl, pr, pt, pb, // padding
                  ml, mr, mt, mb, // margin
                  bl, br, bt, bb, // border
                  radius;
        RGBA *bg = nullptr, *color = nullptr, *border_color = nullptr;
        // children
        union C {
            constexpr C() : children{} {}
            Text text;
            Pixels pixels;
            struct {
                size_t length = 0;
                Box* boxes = nullptr;
            } children;
        };
        enum {
            TEXT,
            PIXELS,
            BOXES
        } child_type;
    };

    struct DisplayBuffer16 {
        uint16_t* pixels = nullptr;
        uint16_t width = 0, height = 0;
    };

    void render(Box* box, DisplayBuffer16* buffer);
}