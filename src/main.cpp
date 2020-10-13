#include <tusb.h>

#include "modm/board.hpp"
#include <modm/math.hpp>
#include <modm/io.hpp>
#include <modm/processing.hpp>
#include <modm/platform.hpp>
#include <modm/driver/display/ili9341_spi.hpp>
#include <modm/driver/touch/ads7843.hpp>

#include "MovingAverage.hpp"
#include "ui/UiManager.hpp"
#include "ui/Numeric.hpp"
#include "ui/ImageButton.hpp"
#include "ui/images/up_arrow.hpp"
#include "ui/images/down_arrow.hpp"
#include "ui/images/play.hpp"
#include "ui/images/stop.hpp"

using namespace modm::platform;

namespace display {
    using Spi = SpiMaster1;
    using Cs = GpioA4;
    using Sck = GpioA5;
    using Miso = GpioA6;
    using Mosi = GpioA7;
    using Dc = GpioB11;
    using Reset = GpioB1;
    using Backlight = GpioB10;
}

namespace touchpins {
    using Spi = SpiMaster1;
    using Cs = GpioA2;
    using Int = GpioA1;
}

namespace pwm {
    using Pin = GpioB0;
    using Timer = Timer3;
    const uint32_t Chan = 3;
}

namespace rpm {
    using Pin = GpioA0;
    using Timer = Timer2;
    const uint32_t Chan = 1;
}

modm::Ili9341Spi<
	display::Spi,
	display::Cs,
	display::Dc,
	display::Reset,
	display::Backlight
> tft;

modm::Ads7843<touchpins::Spi, touchpins::Cs, touchpins::Int> touch;

modm::IODeviceWrapper<UsbUart0, modm::IOBuffer::DiscardIfFull> usb_io_device0;
modm::IOStream usb_stream0(usb_io_device0);

modm::PeriodicTimer timer{1s};

ui::UiManager uiManager(&tft);

ui::Numeric<4> settingNumeric(20, 30, modm::glcd::Color::navy(), modm::glcd::Color::maroon());
ui::Numeric<4> actualNumeric(20, 150, modm::glcd::Color::black(), modm::glcd::Color::maroon());
ui::ImageButton upButton(210, 10, modm::accessor::asFlash(images::up_arrow));
ui::ImageButton downButton(210, 68, modm::accessor::asFlash(images::down_arrow));
ui::ImageButton playButton(220, 150, modm::accessor::asFlash(images::play));
ui::ImageButton stopButton(220, 150, modm::accessor::asFlash(images::stop));

uint16_t rpmSetting = 0;
MovingAverage<20> rpmFilter;
bool rpmValid = false;

void BuildUi() {
    uiManager.addWidget(&settingNumeric);
    uiManager.addWidget(&actualNumeric);
    uiManager.addWidget(&upButton);
    uiManager.addWidget(&downButton);
    uiManager.addWidget(&playButton);
    uiManager.addWidget(&stopButton);

    stopButton.hide();

    upButton.registerClick([]() {
        uint16_t increment = std::pow(10, 3 - settingNumeric.getActiveDigit());
        rpmSetting += increment;
        usb_stream0 << "Up " << increment << "\r\n";
        settingNumeric.setValue(rpmSetting);
    });

    downButton.registerClick([]() {
        uint16_t increment = std::pow(10, 3 - settingNumeric.getActiveDigit());
        rpmSetting -= increment;
        settingNumeric.setValue(rpmSetting);
    });

    playButton.registerClick([]() {

        usb_stream0 << "Play " << "\r\n";
        playButton.hide();
        stopButton.show();
    });

    stopButton.registerClick([]() {
        usb_stream0 << "Stop " << "\r\n";
        stopButton.hide();
        playButton.show();
    });
}

namespace touchCalibration {
    const uint16_t MinX = 390;
    const uint16_t MinY = 360;
    const uint16_t MaxX = 3880;
    const uint16_t MaxY = 3800;
};

void setupPwm() {
    pwm::Pin::setOutput(true);
    pwm::Timer::enable();
    pwm::Timer::connect<pwm::Pin::Ch3>();
    pwm::Timer::setMode(
        pwm::Timer::Mode::UpCounter,
        pwm::Timer::SlaveMode::Disabled
    );
    pwm::Timer::setPeriod<Board::SystemClock>(50 * 1000); // microseconds
    pwm::Timer::configureOutputChannel(
        pwm::Chan,
        pwm::Timer::OutputCompareMode::Pwm,
        0
    );
    pwm::Timer::start();
    pwm::Timer::setNormalPwm(pwm::Chan);
}

void setPulseWidth(uint32_t width_us) {
    float cycles = (float)pwm::Timer::getTickFrequency<Board::SystemClock>() * (float)width_us / 1e6f;
    usb_stream0 << "Setting pulse width: " << cycles << "\r\n";

    pwm::Timer::setCompareValue(pwm::Chan, (uint16_t)(cycles + 0.5));
}

void setupRpm() {
    rpm::Timer::enable();
    rpm::Timer::setMode(
        rpm::Timer::Mode::UpCounter,
        rpm::Timer::SlaveMode::Disabled
    );
    rpm::Timer::configureInputChannel(
        rpm::Chan,
        rpm::Timer::InputCaptureMapping::InputOwn,
        rpm::Timer::InputCapturePrescaler::Div1,
        rpm::Timer::InputCapturePolarity::Rising,
        8
    );
    rpm::Timer::enableInterruptVector(true, 3);
    rpm::Timer::enableInterrupt(rpm::Timer::Interrupt::CaptureCompare1);
    rpm::Timer::enableInterrupt(rpm::Timer::Interrupt::Update);
    rpm::Timer::start();
}

uint32_t getRpm() {
    if(rpmValid) {
        uint32_t freq = rpm::Timer::getTickFrequency<Board::SystemClock>();
        uint32_t period = rpmFilter.get();
        usb_stream0 << "period: " << period << "\r\n";
        if(period > 0) {
            // 7 electrical rev / mechanical rev
            return (uint32_t)((float)60.0 * freq / period / 7);
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

MODM_ISR(TIM2)
{
    static uint32_t highCount; // Count overflows
    static uint32_t lastCapture;
    auto flags = rpm::Timer::getInterruptFlags();
    rpm::Timer::acknowledgeInterruptFlags(flags);

    Board::LedGreen::set();

    uint32_t newCount = rpm::Timer::getCompareValue(rpm::Chan) + highCount * 65536;

    if(flags.value & (uint32_t)rpm::Timer::InterruptFlag::Update) {
        highCount += 1;
        if(newCount - lastCapture > 500000000) {
            rpmValid = false;
        }
    }

    if(flags.value & (uint32_t)rpm::Timer::InterruptFlag::CaptureCompare1) {
        rpmFilter.push(newCount - lastCapture);
        lastCapture = newCount;
        rpmValid = true;
    }

    Board::LedGreen::reset();
}

int main() {
    Board::initialize();

    Board::usb::Dp::setOutput(false);
    modm::delay_ms(1);
    Board::usb::Dp::setInput();
    Board::initializeUsbFs();

    setupPwm();
    setupRpm();

    setPulseWidth(1500);

    display::Spi::connect<display::Sck::Sck, display::Miso::Miso, display::Mosi::Mosi>();
	display::Spi::initialize<Board::SystemClock, 2248_kHz, 20_pct>();
	tft.initialize();
    tft.enableBacklight(true);
    tft.setRotation(modm::ili9341::Rotation::Rotate270);

    touchpins::Cs::setOutput(true);
    touch.initialize();

	Board::LedGreen::set();

	tft.setColor(modm::glcd::Color::red());
    tft.setBackgroundColor(modm::glcd::Color::white());
    tft.clear();
	int16_t w = tft.getWidth();
	int16_t h = tft.getHeight();

    BuildUi();
    uiManager.redraw();
    settingNumeric.setActiveDigit(1);

    tusb_init();

    while(true) {
        tud_task();

        modm::glcd::Point p;
        bool touch_active = touch.testRead(&p);

        // TODO: There's something screwy with the display driver width/height
        // that I've been ignoring for now.
        // I had to swap its width and height to properly clear, but then the
        // reported width/height (as used here) are swapped...
        int16_t px = (p.x - touchCalibration::MinX) * h / (touchCalibration::MaxX - touchCalibration::MinX);
        int16_t py = (p.y - touchCalibration::MinY) * w / (touchCalibration::MaxY - touchCalibration::MinY);
        uiManager.handleTouchStatus(touch_active, px, py);

        if(timer.execute()) {
            Board::LedGreen::toggle();
            setPulseWidth(1500);

            usb_stream0 << "RPM: " << getRpm() << "\r\n";

            actualNumeric.setValue(getRpm());
        }
    }

}