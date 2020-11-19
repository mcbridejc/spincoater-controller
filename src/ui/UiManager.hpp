#pragma once

#include "Widget.hpp"

namespace ui {
class UiManager {
public:
    UiManager(modm::GraphicDisplay *_display) :
        display(_display),
        list(NULL),
        touchActive(false),
        debounceCounter(0)
    {

    }

    void addWidget(Widget *new_widget) {
        new_widget->setDisplay(display);
        
        Widget **last_widget = &list;
        while(*last_widget != NULL) {
            last_widget = &(*last_widget)->next;
        }
        *last_widget = new_widget;
    }

    void redraw() {
        Widget *p = list;
        while(p) {
            if(!p->hidden) {
                p->redraw();
            }
            p = p->next;
        }
    }

    void handleTouchStatus(bool active, int16_t x, int16_t y) {
        if(!touchActive && active && debounceCounter == 0) {
            Widget *p = list;
            while(p) {
                if(x > p->left && x < p->left + p->width && y > p->top && y < p->top + p->height) {
                    if(!p->hidden) {
                        p->onClick(x, y);
                        // Only let one component handle a click event
                        break;
                    }
                }
                p = p->next;
            }
        }
        touchActive = active;
        if(active) {
            debounceCounter = 25;
        }
        if(!active && debounceCounter > 0) {
            debounceCounter--;
        }
    }

private:
    modm::GraphicDisplay *display;
    Widget *list;
    bool touchActive;
    uint8_t debounceCounter;
};

}