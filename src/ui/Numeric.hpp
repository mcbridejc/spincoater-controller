#include "Digit.hpp"

namespace ui {

template <uint8_t N>
class Numeric : public Widget {
public:
    Numeric(
        int16_t left, 
        int16_t top, 
        modm::glcd::Color _color
        ) : Widget(left, top, Digit::WIDTH * N, Digit::HEIGHT)
    {
        digitColor = _color;
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

    void setDisplay(modm::GraphicDisplay *d) {
        display = d;
        for(auto &digit : digits) {
            digit.setDisplay(d);
        }
    }

    void redraw() {
        for(auto &d : digits) {
            d.redraw();
        }
    }
    modm::glcd::Color digitColor;
    Digit digits[N];
};

template<uint8_t N>
class NumericActiveDigit : public Numeric<N> {
public:

    NumericActiveDigit(
        int16_t left, 
        int16_t top, 
        modm::glcd::Color _color, 
        modm::glcd::Color _highlight_color
        ) : 
            Numeric<N>(left, top, _color),
            activeDigit(-1)
    {
        highlightColor = _highlight_color;
    }

    void setActiveDigit(int value) {
        if(value != activeDigit) {
            if(activeDigit >= 0 && activeDigit < N) {
                this->digits[activeDigit].setColor(this->digitColor);
                this->digits[activeDigit].redraw();
            }
            
            activeDigit = value;

            if(value >= 0 && value < N) {
                this->digits[activeDigit].setColor(highlightColor);
                this->digits[activeDigit].redraw();
            }
        }
    }

    uint8_t getActiveDigit() {
        return activeDigit;
    }

    virtual void onClick(int16_t x, int16_t y) override {
        (void)y;
        for(uint8_t i = 0; i<N; i++) {
            if(x < this->left + Digit::WIDTH * (i+1)) {
                setActiveDigit(i);
                return;
            }
        }
    }

protected:
    int activeDigit;
    modm::glcd::Color highlightColor;
};

}