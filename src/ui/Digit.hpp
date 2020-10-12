#pragma once

#include <stdint.h>
#include <modm/ui/display/graphic_display.hpp>

#include "Widget.hpp"

namespace ui {

class Digit : public Widget {
public:
    Digit() :
        Widget(0, 0, WIDTH, HEIGHT),
        value(0),
        color(modm::glcd::Color::black())
    {

    }

    Digit(uint16_t left, uint16_t top, modm::glcd::Color foreColor) : 
        Widget(left, top, WIDTH, HEIGHT),
        value(0),
        color(foreColor)
    {

    }

    void setPosition(int16_t new_left, int16_t new_top) {
        left = new_left;
        top = new_top;
    }

    void setColor(modm::glcd::Color new_color) {
        color = new_color;
    }

    void setValue(uint8_t newValue) {
        if(newValue != value) {
            value = newValue;
            redraw();
        }
    }

    void redraw() {
        if(!display) {
            return;
        }
        display->setFont(&FONT);
        display->setColor(color);
        display->setCursor({left, top});
        display->write(value + '0');
    }

     modm::accessor::Flash<uint8_t> FONT = modm::accessor::asFlash(modm::font::Numbers40x57);
     static const uint8_t WIDTH = 40;
     static const uint8_t HEIGHT = 56;

private:
    uint8_t value;
    modm::glcd::Color color;
};

    
}