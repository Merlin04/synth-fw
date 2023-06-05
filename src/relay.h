/*
 * ReLay: _Re_active _Lay_out
 * (header file - see the cpp file for more details)
 * This file primarily contains struct definitions.
 */

#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <list>

#define SETTER(name, type) Box* name(type name);

#define CSS_SHORTHAND(name) \
    SETTER(name, int16_t) \
    Box* name(int16_t name##y, int16_t name##x); \
    Box* name(int16_t name##t, int16_t name##x, int16_t name##b); \
    Box* name(int16_t name##t, int16_t name##r, int16_t name##b, int16_t name##l); \
    SETTER(name##x, int16_t) \
    SETTER(name##y, int16_t) \
    SETTER(name##t, int16_t) \
    SETTER(name##r, int16_t) \
    SETTER(name##b, int16_t) \
    SETTER(name##l, int16_t)

#define BOX_SETTERS \
        CSS_SHORTHAND(p) \
        CSS_SHORTHAND(m) \
        SETTER(x, int16_t) \
        SETTER(y, int16_t) \
        SETTER(w, int16_t) \
        SETTER(h, int16_t) \
        SETTER(direction, Direction) \
        SETTER(gap, int16_t) \
        SETTER(fill_primary, bool) \
        SETTER(fill_secondary, bool) \
        SETTER(children_fill_primary, bool) \
        SETTER(children_fill_secondary, bool) \
        SETTER(bg, RGB*) \
        SETTER(color, RGB*) \
        SETTER(border_color, RGB*) \
        SETTER(border, int16_t)

// w, h necessary for rendering buffers, even if in flex flow (because we're relying on intrinsic size, and buffer might not have that)

/*
 * updated layout properties
 padding, margin, border color, border thickness, color, font size
 layout:
 direction: sets the primary axis (horizontal or vertical)
 gap: sets the gap between child elements part of the flex flow
 fillPrimary: can only be on one child element, makes the element fill the remaining space on the primary axis, going in the current render direction. If you want to render items after this element, swap the render direction then put this item at the end.
 fillSecondary: can be on any child element, makes the element's secondary axis fill the parent's secondary axis
 childrenFillPrimary: makes all children share the space on the primary axis. Makes fillPrimary do nothing
 childrenFillSecondary: sets default value for children's fillSecondary, can be overridden by fillSecondary on individual children
 x, y, width, height: removes child from flex flow and positions absolutely

 a box either has children, a text property, or a bitmap property which is a callback that takes width/height and returns a pointer to a buffer of pixels to render then delete

 in order for fillPrimary or childrenFillPrimary to be set, the element's primary size must be set somehow (either by the parent (like in fillSecondary) or manually)

 rendering a box returns its x, y, width, height (except if it's a fillPrimary or if childrenFillPrimary is set on the container, you have to use a function after the container is rendered to get the size)
 */

namespace Re {
    // simple color struct
    struct RGB {
        uint8_t r = 0, g = 0, b = 0;
        uint16_t _565;
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
    };

    struct DisplayBuffer16 {
        uint16_t* pixels = nullptr;
        uint16_t width = 0, height = 0;
    };

    /*struct Pixels {
        uint16_t* pixels = nullptr;
        uint16_t width = 0, height = 0;
    };*/

    struct BoundingBox {
        int16_t x, y, w, h;
    };

    enum Direction {
        HORIZONTAL,
        VERTICAL
    };

    // this is the primary primitive that ReLay can render. it can be styled through a variety of attributes
    // and can contain children (either a single text node, a single pixels node, or multiple other `Box`es).
    class Box {
        private:

        int16_t _x = -1, _y = -1, _w = -1, _h = -1,     // position - set to -1 (default) to use flex layout
                _pl, _pr, _pt, _pb, // padding
                _ml, _mr, _mt, _mb, // margin
                _border; // border
        RGB *_bg = nullptr, *_color = nullptr, *_border_color = nullptr;
        Direction _direction = HORIZONTAL;
        int16_t _gap = 0;
        bool _fill_primary = false, _fill_secondary = false,
             _children_fill_primary = false, _children_fill_secondary = false;
        int16_t _child_sizes = -1;
        int16_t _fill_primary_start = -1, _fill_primary_end = -1;
        // children
        /*union {
            const char* _text;
            Pixels _pixels;
            std::list<Box>* _children;
        };
        enum {
            TEXT,
            PIXELS,
            BOXES
        } _child_type;*/

        void _parentDrawBgCallback(BoundingBox* box);
        void _drawSlice(int16_t x, int16_t y, int16_t secondary_size, int16_t primary_size);
        int16_t _primarySize();
        int16_t _secondarySize();
        void _setPrimarySize(int16_t size);
        void _setSecondarySize(int16_t size);
        int16_t _primaryPos();
        int16_t _secondaryPos();
        void _setPrimaryPos(int16_t pos);
        void _setSecondaryPos(int16_t pos);

        public:

        BOX_SETTERS

        Box() {}
        BoundingBox render();
        void render(DisplayBuffer16* buffer);
        void renderAndEnd();
        
        friend void text(const char* text);
        // friend void pixels(uint16_t* pixels, uint16_t width, uint16_t height);
        // void pixels(auto&& lambda);
        friend void swapDirection();
        friend void end();
        // friend void updateXY();
    };

    Box* box();
    void text(const char* text);
    // void pixels(uint16_t* pixels, uint16_t width, uint16_t height);
    void swapDirection();

    void end();
}