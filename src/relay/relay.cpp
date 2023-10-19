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
#include <yoga/Yoga-internal.h>

#define RELAY_DEBUG

namespace Re {
    Box* current_parent;

    RGB black = {0, 0, 0};

    Box* box() {
#ifdef RELAY_DEBUG
        Serial.println("ReLay: box()");
#endif
        auto b = new Box();
        if(!current_parent) return b;

#ifdef RELAY_DEBUG
        Serial.println("ReLay: setting up new box as child");
#endif

        // set up b as child of current, and transfer ownership
        if(current_parent->_child_type != Box::BOXES) {
            // this codepath should not be possible
            Serial.println("ReLay: error: current_parent is a node that does not have box children!!!");
            return b;
        }

        current_parent->_children->emplace_back(b);
        YGNodeInsertChild(current_parent->_node, b->_node, YGNodeGetChildCount(current_parent->_node));

        b->_parent = current_parent;

#ifdef RELAY_DEBUG
        Serial.println("ReLay: returning box");
#endif

        return b;
    }

    Box::Box() {
#ifdef RELAY_DEBUG
        Serial.println("ReLay: Box constructor");
#endif
        _node = YGNodeNew();
#ifdef RELAY_DEBUG
        Serial.println("ReLay: Box constructor done");
#endif
    }

    Box::~Box() {
        // child destructors will be automatically called, since
        // they're unique_ptrs in an std::list

        // for now, we won't "properly" clean up the node (removing
        // it from the parent, etc) because we probably will only
        // call the destructor from when the root is destructed
        // so ig this is a TODO to maybe figure out later
        // (who am i kidding, this won't be fixed will it)

        // anyway
#ifdef RELAY_DEBUG
        Serial.println("ReLay: Box destructor");
#endif
        YGNodeDeallocate(_node); // for proper cleanup use YGNodeFree
#ifdef RELAY_DEBUG
        Serial.println("ReLay: Box destructor done");
#endif
    }

    Box* Box::children() {
#ifdef RELAY_DEBUG
        Serial.println("ReLay: children()");
#endif
        current_parent = this;
        _child_type = BOXES;
        _children = std::make_unique<std::list<std::unique_ptr<Box>>>();
#ifdef RELAY_DEBUG
        Serial.println("ReLay: children() done");
#endif
        return this;
    }

    void end() {
        if(!current_parent) return;
        current_parent = current_parent->_parent;
#ifdef RELAY_DEBUG
        Serial.println("ReLay: end()");
#endif
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
                _border_color = std::make_unique<RGB>(black);
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
                    for(auto &child : *_children) {
                        child->_renderSelf(tft);
                    }
                }
                break;
            case TEXT:
                if(_color == nullptr) {
                    // default to black
                    _color = std::make_unique<RGB>(black);
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
}