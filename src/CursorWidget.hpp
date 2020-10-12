#include <modm/ui/display/graphic_display.hpp>

class CursorWidget {
public:
    CursorWidget(modm::GraphicDisplay *_display) : display(_display), active(false) {}

    void setPos(uint16_t x, uint16_t y) {
        if(active) {
            clear();
        }
        
        display->setColor(modm::glcd::Color::red());
        location.x = x;
        location.y = y;
        display->fillRectangle({location.x-Size, location.y-Size}, Size*2, Size*2);

        active = true;
    }

    void clear() {
        display->setColor(modm::glcd::Color::white());
        display->fillRectangle({location.x-Size, location.y-Size}, Size*2, Size*2);
        active = false;
    }

private:
    modm::GraphicDisplay *display;
    bool active;
    modm::glcd::Point location;

    static const int Size = 4;
};