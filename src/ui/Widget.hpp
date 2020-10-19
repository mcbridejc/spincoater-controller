#pragma once

#include <stdint.h>
#include <modm/ui/display.hpp>

namespace ui {

class UiManager;

class Widget {
public:
Widget() : 
        left(0),
        top(0),
        width(0),
        height(0),
        next(NULL)
    {
    }

    Widget(int16_t _left, int16_t _top, int16_t _width, int16_t _height) :
        left(_left),
        top(_top),
        width(_width),
        height(_height),
        display(NULL),
        next(NULL)
    {
    }

    virtual void redraw() = 0;

    virtual void onClick(int16_t x, int16_t y) {
        (void)x;
        (void)y;
    }

    int16_t left;
    int16_t top;
    int16_t width;
    int16_t height;

    virtual void setDisplay(modm::GraphicDisplay *d) {
        display = d;
    }

    virtual void hide() {
        if(display && !hidden) {
            display->setColor(modm::glcd::Color::white());
            display->fillRectangle(left, top, width, height);
        }

        hidden = true;
    }

    virtual void show() {
        if(display && hidden) {
            redraw();
        }
        hidden = false;
    }

protected:
    modm::GraphicDisplay *display;
    friend class UiManager;
    bool hidden;
private:
    Widget *next;
};

} // namespace ui
