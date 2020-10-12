#pragma once

#include "Widget.hpp"

namespace ui {

class Target : public Widget {
public:
    Target(int16_t x, int16_t y) : 
        Widget(x - SIZE/2, y-SIZE/2, SIZE, SIZE),
        centerX(x),
        centerY(y)
    {

    }

    void redraw() {
        if(!display) {
            return;
        }
        display->drawCircle({centerX, centerY}, SIZE/2);
        display->drawLine({centerX - SIZE/2, centerY - SIZE/2}, {centerX + SIZE/2, centerY + SIZE/2});
        display->drawLine({centerX + SIZE/2, centerY - SIZE/2}, {centerX - SIZE/2, centerY + SIZE/2});
    }

private:
    int16_t centerX;
    int16_t centerY;
    static const uint16_t SIZE = 40;
};

} //namespace ui
