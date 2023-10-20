/*
 * ReLay: _Re_active _Lay_out
 * (header file - see the cpp file for more details)
 * This file primarily contains struct definitions.
 */

#pragma once
#include <cstdint>
#include <cstdlib>
#include <list>
#include <Adafruit_GFX.h>
#include <ST7789_t3.h>
#include <memory>

#include "yoga/Yoga.h"

#define SETTER(name, type) Box* name(const std::unique_ptr<type> name) { \
    this->_##name = std::unique_ptr<RGB>(new RGB(name.get())); \
    return this; \
}

#define COLOR_SETTER(name) \
    SETTER(name, RGB)     \
    Box* name(uint8_t r, uint8_t g, uint8_t b) { this->_##name = std::make_unique<RGB>(r, g, b); return this; }                           \
//    Box* name(RGB name) { this->_##name = new RGB(name); return this; }

#define CSS_SHORTHAND(name, ygName) \
    Box* name(float name) { YGNodeStyleSet##ygName(_node, YGEdgeAll, name); return this; } \
    Box* name(float name##y, float name##x) { YGNodeStyleSet##ygName(_node, YGEdgeVertical, name##y); YGNodeStyleSet##ygName(_node, YGEdgeHorizontal, name##x); return this; } \
    Box* name(float name##t, float name##x, float name##b) { YGNodeStyleSet##ygName(_node, YGEdgeTop, name##t); YGNodeStyleSet##ygName(_node, YGEdgeHorizontal, name##x); YGNodeStyleSet##ygName(_node, YGEdgeBottom, name##b); return this; } \
    Box* name(float name##t, float name##r, float name##b, float name##l) { YGNodeStyleSet##ygName(_node, YGEdgeTop, name##t); YGNodeStyleSet##ygName(_node, YGEdgeRight, name##r); YGNodeStyleSet##ygName(_node, YGEdgeBottom, name##b); YGNodeStyleSet##ygName(_node, YGEdgeLeft, name##l); return this; } \
    Box* name##x(float name##x) { YGNodeStyleSet##ygName(_node, YGEdgeHorizontal, name##x); return this; } \
    Box* name##y(float name##y) { YGNodeStyleSet##ygName(_node, YGEdgeVertical, name##y); return this; } \
    Box* name##t(float name##t) { YGNodeStyleSet##ygName(_node, YGEdgeTop, name##t); return this; } \
    Box* name##r(float name##r) { YGNodeStyleSet##ygName(_node, YGEdgeRight, name##r); return this; } \
    Box* name##b(float name##b) { YGNodeStyleSet##ygName(_node, YGEdgeBottom, name##b); return this; } \
    Box* name##l(float name##l) { YGNodeStyleSet##ygName(_node, YGEdgeLeft, name##l); return this; }

#define BOX_SETTERS \
        CSS_SHORTHAND(p, Padding) \
        CSS_SHORTHAND(m, Margin) \
        COLOR_SETTER(bg) \
        COLOR_SETTER(color) \
        COLOR_SETTER(border_color) \

namespace Re {
    // simple color struct
    struct RGB {
        uint8_t r = 0, g = 0, b = 0;
        uint16_t _565 = 0;
        bool _565_valid = false;
        
        uint16_t to565() {
            if(_565_valid) return _565;
            _565_valid = true;
            return _565 = (r & 0xF8) << 8 | (g & 0xFC) << 3 | (b & 0xF8) >> 3;
        }

        static RGB from565(uint16_t color) {
            return {
                (uint8_t) ((color & 0xF800) >> 8),
                (uint8_t) ((color & 0x07E0) >> 3),
                (uint8_t) ((color & 0x001F) << 3)
            };
        }

        RGB() = default;
        RGB(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}
        explicit RGB(RGB* rgb) : r(rgb->r), g(rgb->g), b(rgb->b) {}
    };

    // pixels callback function - it takes an x, y, width, height, and buffer pointer, and returns void
    typedef void (*pixels_cb)(uint16_t, uint16_t, uint16_t, uint16_t, ST7789_t3*);

    struct BoundingBox {
        int16_t x, y, w, h;
    };

    // this is the primary primitive that ReLay can render. it can be styled through a variety of attributes
    // and can contain children (either a single text node, a single pixels node, or multiple other `Box`es).
    class Box {
        private:

        YGNodeRef _node = nullptr;

        std::unique_ptr<RGB> _bg = nullptr, _color = nullptr, _border_color = nullptr;
        // children
        union {
            const char* _text;
            pixels_cb _pixels_cb;
            std::unique_ptr<std::list<std::unique_ptr<Box>>> _children = std::make_unique<std::list<std::unique_ptr<Box>>>();
        };
        enum {
            TEXT,
            PIXELS,
            BOXES
        } _child_type = BOXES;

        Box* _parent = nullptr; // pointer/this' memory managed by parent

        Box();

        void _renderSelf(ST7789_t3* tft);

        public:

        ~Box(); // needs to be public so std::unique_ptr can call

        //        void render(DisplayBuffer16* buffer);
        void render(ST7789_t3* tft);

        Box* text(const char* text);
        Box* pixels(pixels_cb cb);
        Box* children();
        friend Box* box();
        friend void end();

#define YG_PROP_FN(name, fnName, type) \
    Box* name(const type name) { YGNodeStyleSet##fnName(_node, name); return this; }

#define YG_PROP_FN_0A(name, fnName) \
    Box* name() { YGNodeStyleSet##fnName(_node); return this; }

#define YG_PROP_FN_1A(name, fnName, arg0, arg0Type) \
    Box* name(const arg0Type arg0) { YGNodeStyleSet##fnName(_node, arg0); return this; }

#define YG_PROP_FN_2A(name, fnName, arg0, arg0Type, arg1, arg1Type) \
    Box* name(const arg0Type arg0, const arg1Type arg1) { YGNodeStyleSet##fnName(_node, arg0, arg1); return this; }
        
        YG_PROP_FN(direction, Direction, YGDirection)
        YG_PROP_FN(flexDirection, FlexDirection, YGFlexDirection)
        YG_PROP_FN(justifyContent, JustifyContent, YGJustify)
        YG_PROP_FN(alignContent, AlignContent, YGAlign)
        YG_PROP_FN(alignItems, AlignItems, YGAlign)
        YG_PROP_FN(alignSelf, AlignSelf, YGAlign)
        YG_PROP_FN(positionType, PositionType, YGPositionType)
        YG_PROP_FN(flexWrap, FlexWrap, YGWrap)
        YG_PROP_FN(overflow, Overflow, YGOverflow)
        YG_PROP_FN(display, Display, YGDisplay)
        YG_PROP_FN(flex, Flex, float)
        YG_PROP_FN(flexGrow, FlexGrow, float)
        YG_PROP_FN(flexShrink, FlexShrink, float)
        YG_PROP_FN(flexBasis, FlexBasis, float)
        YG_PROP_FN(flexBasisPercent, FlexBasisPercent, float)
        YG_PROP_FN_0A(flexBasisAuto, FlexBasisAuto)
        YG_PROP_FN_2A(position, Position, edge, YGEdge, points, float)
        YG_PROP_FN_2A(positionPercent, PositionPercent, edge, YGEdge, percent, float)
        YG_PROP_FN_2A(margin, Margin, edge, YGEdge, points, float)
        YG_PROP_FN_2A(marginPercent, MarginPercent, edge, YGEdge, percent, float)
        YG_PROP_FN_1A(marginAuto, MarginAuto, edge, YGEdge)
        YG_PROP_FN_2A(padding, Padding, edge, YGEdge, points, float)
        YG_PROP_FN_2A(paddingPercent, PaddingPercent, edge, YGEdge, percent, float)
        YG_PROP_FN_2A(border, Border, edge, YGEdge, border, float)
        YG_PROP_FN_2A(gap, Gap, gutter, YGGutter, gapLength, float)
        YG_PROP_FN(aspectRatio, AspectRatio, float)
        YG_PROP_FN_1A(width, Width, points, float)
        YG_PROP_FN_1A(widthPercent, WidthPercent, percent, float)
        YG_PROP_FN_0A(widthAuto, WidthAuto)
        YG_PROP_FN_1A(height, Height, points, float)
        YG_PROP_FN_1A(heightPercent, HeightPercent, percent, float)
        YG_PROP_FN_0A(heightAuto, HeightAuto)
        YG_PROP_FN(minWidth, MinWidth, float)
        YG_PROP_FN(minWidthPercent, MinWidthPercent, float)
        YG_PROP_FN(minHeight, MinHeight, float)
        YG_PROP_FN(minHeightPercent, MinHeightPercent, float)
        YG_PROP_FN(maxWidth, MaxWidth, float)
        YG_PROP_FN(maxWidthPercent, MaxWidthPercent, float)
        YG_PROP_FN(maxHeight, MaxHeight, float)
        YG_PROP_FN(maxHeightPercent, MaxHeightPercent, float)

        BOX_SETTERS
    };

    Box* box();
    void end();

    //    struct DisplayBuffer16 {
//        uint16_t* pixels = nullptr;
//        uint16_t width = 0, height = 0;
//
//        void (*drawString)(const char* text, uint8_t size, uint16_t x, uint16_t y, uint16_t color);
//    };

//    class DisplayST7789 {
//        ST7789_t3 tft;
//    };
}

#define MAKE_RGB(r, g, b) std::make_unique<Re::RGB>(r, g, b)