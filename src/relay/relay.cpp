/*
 * ReLay: _Re_active _Lay_out
 * A simple immediate mode layout library for modern C++, inspired by React and Dear Imgui.
 * Lightweight and fast enough to run on an MCU, and without a built-in input handling system.
 * At its heart, it renders boxes with a set of styles, as well as text nodes and pixel data.
 * Additional layers add additional features (like block layout and state management) and a factory pattern for components.
 */

#include "relay.hpp"
#include <stack>
#include <Arduino.h>

namespace Re {
    // stores properties that get passed down to children, and others that are necessary for rendering
    // struct Context {
    //     Box* parent;
    // };

    // std::stack<Context*> c_stack;
    // Context* current;
    Box* current;

    RGB black = {0, 0, 0};
    
//    void drawHorizontalLine(DisplayBuffer16* buf, uint16_t x, uint16_t y, uint16_t length, uint16_t color) {
//        auto offset = y * buf->width + x;
//        for (uint16_t i = offset; i < offset + length; i++) {
//            buf->pixels[i] = color;
//        }
//    }
//
//    void drawVerticalLine(DisplayBuffer16* buf, uint16_t x, uint16_t y, uint16_t length, uint16_t color) {
//        auto offset = y * buf->width + x;
//        for (uint16_t i = offset; i < offset + length * buf->width; i += buf->width) {
//            buf->pixels[i] = color;
//        }
//    }
//
//    void fillRect(DisplayBuffer16* buf, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
//        auto offset = y * buf->width + x;
//        for (uint16_t i = offset; i < offset + h * buf->width; i += buf->width) {
//            for (uint16_t j = i; j < i + w; j++) {
//                buf->pixels[j] = color;
//            }
//        }
//    }

    void Box::_init() {
        // Run on object instantiation.
        if(!current) return;
        // inherit properties that should be inherited
        // (as of right now, just text color)
        _color = current->_color;

        // add to children of current
        if(current->_child_type == BOXES) {
            if(!current->_children) current->_children = new std::list<Box*>();
            current->_children->push_back(this);

            YGNodeInsertChild(current->_node, _node, YGNodeGetChildCount(current->_node));
        }
        // the user should never add children to a box that has text or pixels, in that case we just won't end up rendering the child

        _parent = current;
    }

    Box* Box::children() {
        current = this;
        _child_type = BOXES;
        return this;
    }

    void end() {
        if(!current || !current->_parent) return;
        current = current->_parent;
    }

    Box* Box::text(const char* text) {
        // TODO: figure out what width is set to if user never explicitly set it
        Serial.printf("debug: node width %d\n", YGNodeStyleGetWidth(_node).value);
        // if(YGNodeStyleGetWidth(_node).value == 0) {
        // check if nan
        if(YGNodeStyleGetWidth(_node).value != YGUndefined) {
            // set the width to width of text
            // todo: font size - we'll just assume 9px
            YGNodeStyleSetMinWidth(_node, strlen(text) * 11);
        }
        if(YGNodeStyleGetHeight(_node).value == YGUndefined) {
            // set the height to height of text
            // again TODO font size
            YGNodeStyleSetMinHeight(_node, 9);
        }
        _child_type = TEXT;
        _text = text;
        return this;
    }

    Box* Box::pixels(pixels_cb cb) {
        _child_type = PIXELS;
        _pixels_cb = cb;
        return this;
    }

    void Box::render(ST7789_t3* tft) {
        Serial.println("DEBUG calculate layout");
        Serial.printf("DEBUG width %d height %d\n", tft->width(), tft->height());
        YGNodeCalculateLayout(_node, tft->width(), tft->height(), YGDirectionLTR);
        Serial.println("DEBUG renderself");
        _renderSelf(tft);
    }

    void Box::_renderSelf(ST7789_t3* tft) {
        Serial.println("DEBUG renderself");
        auto left = YGNodeLayoutGetLeft(_node);
        auto top = YGNodeLayoutGetTop(_node);
        auto width = YGNodeLayoutGetWidth(_node);
        auto height = YGNodeLayoutGetHeight(_node);
        Serial.printf("DEBUG left %d top %d width %d height %d\n", left, top, width, height);

        // background
        if(_bg != nullptr) {
            Serial.println("drawing background");
            tft->fillRect(left, top, width, height, _bg->to565());
        }
        Serial.println("DEBUG draw border");

        // draw border
        auto borderLeft = YGNodeLayoutGetBorder(_node, YGEdgeLeft);
        auto borderTop = YGNodeLayoutGetBorder(_node, YGEdgeTop);
        auto borderRight = YGNodeLayoutGetBorder(_node, YGEdgeRight);
        auto borderBottom = YGNodeLayoutGetBorder(_node, YGEdgeBottom);

        if(borderLeft > 0 || borderRight > 0 || borderTop > 0 || borderBottom > 0) {
            if(_border_color == nullptr) {
                // default to black
                _border_color = &black;
            }

            if(borderLeft > 0) {
                tft->fillRect(left, top, borderLeft, height, _border_color->to565());
            }
            if(borderRight > 0) {
                tft->fillRect(left + width - borderRight, top, borderRight, height, _border_color->to565());
            }
            if(borderTop > 0) {
                tft->fillRect(left, top, width, borderTop, _border_color->to565());
            }
            if(borderBottom > 0) {
                tft->fillRect(left, top + height - borderBottom, width, borderBottom, _border_color->to565());
            }
        }

        Serial.println("DEBUG draw children");

        // draw children
        switch(_child_type) {
            case BOXES:
                if(_children != nullptr) {
                    for(auto child : *_children) {
                        child->_renderSelf(tft);
                    }
                }
                break;
            case TEXT:
                if(_color == nullptr) {
                    // default to black
                    _color = &black;
                }
                // TODO: font size
                tft->setTextColor(_color->to565());
                tft->drawString1(const_cast<char *>(_text), strlen(_text), left + YGNodeLayoutGetPadding(_node, YGEdgeLeft), top + YGNodeLayoutGetPadding(_node, YGEdgeTop));
                break;
            case PIXELS:
                if(_pixels_cb != nullptr) {
                    _pixels_cb(left, top, width, height, tft);
                }
                break;
        }
    }

    Box* box() {
        return new Box();
    }
}