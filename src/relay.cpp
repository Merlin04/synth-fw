/*
 * ReLay: _Re_active _Lay_out
 * A simple immediate mode layout library for modern C++, inspired by React and Dear Imgui.
 * Lightweight and fast enough to run on an MCU, and without a built-in input handling system.
 * At its heart, it renders boxes with a set of styles, as well as text nodes and pixel data.
 * Additional layers add additional features (like block layout and state management) and a factory pattern for components.
 */

#include "relay.h"

namespace Re {
    struct StyleContext {
        RGBA color = {0, 0, 0, 1.0f};
    };

    void render(Box* box, Box* parent, DisplayBuffer16* buffer) {
        // render a box using something kinda like the css border-box sizing
        box->x.resolve(parent->w.value); box->y.resolve(parent->h.value);
        box->w.resolve(parent->w.value); box->h.resolve(parent->h.value);
        box->pl.resolve(box->w.value); box->pr.resolve(box->w.value); box->pt.resolve(box->h.value); box->pb.resolve(box->h.value);
        box->ml.resolve(box->w.value); box->mr.resolve(box->w.value); box->mt.resolve(box->h.value); box->mb.resolve(box->h.value);
        box->bl.resolve(box->w.value); box->br.resolve(box->w.value); box->bt.resolve(box->h.value); box->bb.resolve(box->h.value);
        box->radius.resolve(box->w.value); box->radius.resolve(box->h.value);

        // first, draw border 
        if(box->border_color != nullptr) {
            // idk what I'm doing this was written by copilot I need to go eeb and actually do this in a reasonable way later
            // draw border (keeping in mind border radius - don't want to draw over that area)
            for(uint16_t x = box->x.value + box->bl.value; x < box->x.value + box->w.value - box->br.value; x++) {
                for(uint16_t y = box->y.value + box->bt.value; y < box->y.value + box->h.value - box->bb.value; y++) {
                    size_t buffer_pos = y * buffer->width + x;
                    RGBA* c = box->border_color->a == 1.0f ? box->border_color : &(box->border_color->applyAlpha(buffer_pos));
                    buffer->pixels[y * buffer->width + x] = c->to565();
                }
            }

            if(box->radius.value != 0) {
                // draw rounded corners

            }
        }

        if(box->bg != nullptr) {
            // draw bg (keeping in mind border radius - don't want to draw over that area)

        }
    }

    void render(Box* box, DisplayBuffer16* buffer) {
        render(box, nullptr, buffer);
    }
}