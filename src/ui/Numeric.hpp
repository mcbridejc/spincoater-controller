#include "Digit.hpp"

namespace ui {

template <uint8_t N>
class Numeric : public Widget {
public:
    Numeric(
        int16_t left, 
        int16_t top, 
        modm::glcd::Color _color, 
        modm::glcd::Color _highlight_color
        ) : 
            Widget(left, top, Digit::WIDTH * N, Digit::HEIGHT),
            activeDigit(-1)
    {
        digitColor = _color;
        highlightColor = _highlight_color;
        for(uint8_t i=0; i<N; i++) {
            digits[i].setPosition(left + Digit::WIDTH * i, top);
            digits[i].setColor(digitColor);
        }
    }

    void setValue(uint16_t value) {
        int divisor = 1;
        for(uint8_t digit=0; digit<N; digit++) {
            uint8_t x = (value / divisor) % 10;
            digits[N - 1 - digit].setValue(x);
            divisor *= 10;
        }
    }

    void setActiveDigit(int value) {
        if(value != activeDigit) {
            if(activeDigit >= 0 && activeDigit < N) {
                digits[activeDigit].setColor(digitColor);
                digits[activeDigit].redraw();
            }
            
            activeDigit = value;

            if(value >= 0 && value < N) {
                digits[activeDigit].setColor(highlightColor);
                digits[activeDigit].redraw();
            }
        }
    }

    uint8_t getActiveDigit() {
        return activeDigit;
    }

    void setDisplay(modm::GraphicDisplay *d) {
        display = d;
        for(auto &digit : digits) {
            digit.setDisplay(d);
        }
    }

    virtual void onClick(int16_t x, int16_t y) override {
        (void)y;
        for(uint8_t i = 0; i<N; i++) {
            if(x < left + Digit::WIDTH * (i+1)) {
                setActiveDigit(i);
                return;
            }
        }
    }

    void redraw() {
        for(auto &d : digits) {
            d.redraw();
        }
    }
private:
    modm::glcd::Color digitColor;
    modm::glcd::Color highlightColor;
    Digit digits[N];
    int activeDigit;
};

}