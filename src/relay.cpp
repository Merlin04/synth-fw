/*
 * ReLay: _Re_active _Lay_out
 * A simple immediate mode layout library for modern C++, inspired by React and Dear Imgui.
 * Lightweight and fast enough to run on an MCU, and without a built-in input handling system.
 * At its heart, it renders boxes with a set of styles, as well as text nodes and pixel data.
 * Additional layers add additional features (like block layout and state management) and a factory pattern for components.
 */

#include "relay.h"
#include <stack>
#include <vector>

namespace Re {
    enum RenderDirection {
        STE, // start to end
        ETS // end to start
    };
    // stores properties that get passed down to children, and others that are necessary for rendering
    struct Context {
        Box* parent;
        RenderDirection renderDirection;
        // int16_t p_pos, s_pos; // primary position, secondary position
        // int16_t _x_pos, _y_pos; // updated by updateXY
        int16_t x_pos, y_pos; // current cursor position to render new items
        // uint16_t parent_w, parent_h;
        std::vector<int16_t>* child_secondary_sizes;
    };

    std::stack<Context*> c_stack;
    Context* current;
    DisplayBuffer16* buf;


    #define SETTER(name, type) Box* Box::##name(type name) { this->_##name = name; return this; }
    #define CSS_SHORTHAND(name) \
        Box* Box::##name(int16_t name) { this->_##name##l = this->_##name##r = this->_##name##t = this->_##name##b = name; return this; } \
        Box* Box::##name(int16_t name##y, int16_t name##x) { this->_##name##t = this->_##name##b = name##y; this->_##name##l = this->_##name##r = name##x; return this; } \
        Box* Box::##name(int16_t name##t, int16_t name##x, int16_t name##b) { this->_##name##t = name##t; this->_##name##b = name##b; this->_##name##l = this->_##name##r = name##x; return this; } \
        Box* Box::##name(int16_t name##t, int16_t name##r, int16_t name##b, int16_t name##l) { this->_##name##t = name##t; this->_##name##r = name##r; this->_##name##b = name##b; this->_##name##l = name##l; return this; } \
        Box* Box::##name##x(int16_t name##x) { this->_##name##l = this->_##name##r = name##x; return this; } \
        Box* Box::##name##y(int16_t name##y) { this->_##name##t = this->_##name##b = name##y; return this; } \
        SETTER(name##t, int16_t) \
        SETTER(name##r, int16_t) \
        SETTER(name##b, int16_t) \
        SETTER(name##l, int16_t)
    
    BOX_SETTERS

    // void updateXY() {
    //     switch(current->parent->_direction) {
    //         case HORIZONTAL:
    //             current->_x_pos = current->p_pos;
    //             current->_y_pos = current->s_pos;
    //             break;
    //         case VERTICAL:
    //             current->_x_pos = current->s_pos;
    //             current->_y_pos = current->p_pos;
    //             break;
    //     }
    // }

    void drawHorizontalLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color) {
        auto offset = y * buf->width + x;
        for (uint16_t i = offset; i < offset + length; i++) {
            buf->pixels[i] = color;
        }
    }

    void drawVerticalLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color) {
        auto offset = y * buf->width + x;
        for (uint16_t i = offset; i < offset + length * buf->width; i += buf->width) {
            buf->pixels[i] = color;
        }
    }

    void fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
        auto offset = y * buf->width + x;
        for (uint16_t i = offset; i < offset + h * buf->width; i += buf->width) {
            for (uint16_t j = i; j < i + w; j++) {
                buf->pixels[j] = color;
            }
        }
    }

    void Box::_drawSlice(int16_t x, int16_t y, int16_t secondary_size, int16_t primary_size) {
        // draw a "slice" of border and empty space in the direction corresponding to this' layout direction
        // this is used for rendering gap and padding
        if(this->_direction == HORIZONTAL) {
            fillRect(x, y, primary_size, this->_border, this->_border_color->to565()); // top border
            fillRect(x, y + secondary_size - this->_border, primary_size, this->_border, this->_border_color->to565()); // bottom border
            fillRect(x, y + this->_border, primary_size, secondary_size - 2 * this->_border, this->_bg->to565()); // bg
        } else {
            fillRect(x, y, this->_border, secondary_size, this->_border_color->to565()); // left border
            fillRect(x + primary_size - this->_border, y, this->_border, secondary_size, this->_border_color->to565()); // right border
            fillRect(x + this->_border, y, primary_size - 2 * this->_border, secondary_size, this->_bg->to565()); // bg
        }
    }

    /*
     * 4 scenarios we need to consider when drawing the secondary axis bg:
     * 1. We know the secondary size of the parent, and the child should be its intrinsic size
     *    in this case, it's easy since we can just calculate the size from the returned BoundingBox and the sec. size
     * 2. We know the secondary size of the parent, and the child should fill the secondary axis
     *    in this case, it's even easier - we just don't draw any bg since there's no empty secondary axis to fill
     * 3. We don't know the secondary size of the parent, and the child should be its intrinsic size
     *    in this case, we need to draw all of the children first, then keep track of the secondary size of each child,
     *    and fill in the remaining space after all children rendered (since we'll then know the secondary size of parent)
     * 4. We don't know the secondary size of the parent, and the child should fill the secondary axis
     *    this makes no sense since then we have nothing to base the secondary size of the parent on. error condition!
     */

    int16_t Box::_primarySize() {
        return this->_direction == HORIZONTAL ? this->_w : this->_h;
    }
    int16_t Box::_secondarySize() {
        return this->_direction == HORIZONTAL ? this->_h : this->_w;
    }
    void Box::_setPrimarySize(int16_t size) {
        if(this->_direction == HORIZONTAL) {
            this->_w = size;
        } else {
            this->_h = size;
        }
    }
    void Box::_setSecondarySize(int16_t size) {
        if(this->_direction == HORIZONTAL) {
            this->_h = size;
        } else {
            this->_w = size;
        }
    }

    int16_t Box::_primaryPos() {
        return this->_direction == HORIZONTAL ? this->_x : this->_y;
    }
    int16_t Box::_secondaryPos() {
        return this->_direction == HORIZONTAL ? this->_y : this->_x;
    }
    void Box::_setPrimaryPos(int16_t pos) {
        if(this->_direction == HORIZONTAL) {
            this->_x = pos;
        } else {
            this->_y = pos;
        }
    }
    void Box::_setSecondaryPos(int16_t pos) {
        if(this->_direction == HORIZONTAL) {
            this->_y = pos;
        } else {
            this->_x = pos;
        }
    }


    // returning the struct instead of pointer is less efficient but it means we don't have to manually free each one by hand
    BoundingBox Box::render() {
        // note: when a child renders, it also has to draw its contained portion of the parent's background
        // so just inherit the bg
        // and after every child is rendered we need to draw the outside area corresponding to the parent's bg


        if(current->parent->_children_fill_primary) {
            this->_setPrimarySize(current->parent->_child_sizes);
        } else if(this->_fill_primary) {
            // fill the remaining space (everything between the current primary position and the parent)
            this->_setPrimarySize(current->parent->_fill_primary_end - current->parent->_fill_primary_start);
        }

        if(current->parent->_children_fill_secondary) {
            // this->
        }

        this->_x = current->x_pos;
        this->_y = current->y_pos;

        c_stack.push(current);
        current = new Context{
            .parent = this,
            .renderDirection = STE,
            .x_pos = int16_t(current->x_pos + this->_ml + this->_pl),
            .y_pos = int16_t(current->y_pos + this->_mt + this->_pt),
            .child_secondary_sizes = nullptr
            // .parent_w = current->parent_w,
            // .parent_h = current->parent_h
        };
        // first, we have to figure out the primary and secondary sizes of this element.

        // increment for the padding at the start of the primary axis of this box
        if(this->_direction == HORIZONTAL) {
            current->x_pos += this->_pl;
        } else {
            current->y_pos += this->_pt;
        }

        // draw border and padding section corresponding to this' layout direction (start of primary axis)
        // side's border
        if(this->_direction == HORIZONTAL) {

        }
    }

    // this is called on the parent after a render to allow it to draw its portion of the background before moving onto the next child
    void Box::_parentDrawBgCallback(BoundingBox* box) {

    }

    void Box::render(DisplayBuffer16* buffer) {
        buf = buffer;
        current = new Context{
            .parent = nullptr,
            .renderDirection = STE,
            .x_pos = 0,
            .y_pos = 0,
            // .parent_w = buf->width,
            // .parent_h = buf->height
        };
        render();
        delete current;
    }

    Box* box() {
        return new Box();
    }

    void text(const char* text) {
        // todo
    }

    /*void pixels(uint16_t* pixels, uint16_t width, uint16_t height) {
        // todo
    }*/

    void swapDirection() {
        current->renderDirection = current->renderDirection == STE ? ETS : STE;
        // todo
    }

    void end() {
        // todo
    }

    // make it easier to call end with no children
    void Box::renderAndEnd() {
        render();
        end();
    }
}