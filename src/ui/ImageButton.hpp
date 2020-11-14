#include <functional>
#include <modm/architecture/interface/accessor.hpp>
#include "Widget.hpp"

namespace ui {

class ImageButton : public Widget{
public:

    ImageButton(int16_t _left, int16_t _top, modm::accessor::Flash<uint16_t> _image, int16_t _padding=0) {
        left = _left;
        top = _top;
        image = _image;
        padding = _padding;
        width = image[0] + padding * 2;
        height = image[1] + padding * 2;
    }

    void redraw() {
        display->drawBitmap(
            {(int16_t)(left+padding), (int16_t)(top+padding)},
            width - padding * 2,
            height - padding * 2,
            modm::accessor::asFlash<uint8_t>((uint8_t*)&(image.getPointer()[2]))
        );
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
    int16_t padding;
    modm::accessor::Flash<uint16_t> image;
    std::function<void()> clickCallback;
};

} // namespace ui
