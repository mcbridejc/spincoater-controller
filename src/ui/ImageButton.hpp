#include <functional>
#include <modm/architecture/interface/accessor.hpp>
#include "Widget.hpp"

namespace ui {

class ImageButton : public Widget{
public:

    ImageButton(int16_t _left, int16_t _top, modm::accessor::Flash<uint16_t> _image) {
        left = _left;
        top = _top;
        image = _image;
        width = image[0];
        height = image[1];
    }

    void redraw() {
        display->drawBitmap({left, top}, width, height, modm::accessor::asFlash<uint8_t>((uint8_t*)&(image.getPointer()[2])));
    }

    virtual void onClick(int16_t x, int16_t y) override {
        (void)x;
        (void)y;
        if(clickCallback) {
            clickCallback();
        }
    }

    void registerClick(std::function<void()> cb) {
        clickCallback = cb;
    }

private:
    modm::accessor::Flash<uint16_t> image;
    std::function<void()> clickCallback;
};

} // namespace ui
