
#include "modm/board.hpp"
#include <modm/math.hpp>
#include <modm/io.hpp>
#include <modm/processing.hpp>
#include <modm/platform.hpp>
#include <modm/driver/display/ili9341_spi.hpp>

#include "AnalogFrequencyCounter.hpp"
#include "DigitalFrequencyCounter.hpp"
#include "MotorControl.hpp"
#include "MovingAverage.hpp"
#include "xpt2046.hpp"
#include "ui/UiManager.hpp"
#include "ui/Numeric.hpp"
#include "ui/ImageButton.hpp"
#include "ui/images/up_arrow.hpp"
#include "ui/images/down_arrow.hpp"
#include "ui/images/play.hpp"
#include "ui/images/stop.hpp"

using namespace modm::platform;

// Select between the two motor control options:
// 1) If PWM_ESC_CONTROL is defined, a PWM signal is output to control an RC ESC
// using a PID control loop for velocitt
// 2) If PWM_ESC_CONTROL is not defined, the control line is connected to
//#define PWM_ESC_CONTROL

namespace display {
    using Spi = SpiMaster1;
    using Cs = GpioB0;
    using Sck = GpioA5;
    using Miso = GpioA6;
    using Mosi = GpioA7;
    using Dc = GpioF1;
    using Reset = GpioUnused;
    using Backlight = GpioUnused;
}

namespace touchpins {
    using Spi = SpiMaster1;
    using Cs = GpioA8;
    using Int = GpioUnused;
}

#ifdef PWM_ESC_CONTROL
namespace motor {
    using Pin = GpioA9;
    using Timer = Timer1;
    const uint32_t Chan = 2;
    typedef Pin::Ch2<modm::platform::Peripheral::Tim1> ConnectType;
}
#else
namespace motor {
    using Uart = Usart1;
    using Pin = GpioA9;
    typedef Pin::Tx<modm::platform::Peripheral::Usart1> ConnectType;
}
#endif
modm::Ili9341Spi<
	display::Spi,
	display::Cs,
	display::Dc,
	display::Reset,
	display::Backlight
> tft;

Xpt2046<touchpins::Spi, touchpins::Cs, touchpins::Int> touch;

#ifdef PWM_ESC_CONTROL
modm::PeriodicTimer motorTimer{0.02s};
#else
modm::PeriodicTimer motorTimer{0.1s};
#endif
modm::PeriodicTimer touchTimer{0.005s};

ui::UiManager uiManager(&tft);
ui::NumericActiveDigit<4> settingNumeric(20, 30, modm::glcd::Color::navy(), modm::glcd::Color::maroon());
ui::Numeric<4> actualNumeric(20, 150, modm::glcd::Color::black());
ui::ImageButton upButton(210, 0, modm::accessor::asFlash(images::up_arrow), 10);
ui::ImageButton downButton(210, 60, modm::accessor::asFlash(images::down_arrow), 10);
ui::ImageButton playButton(210, 140, modm::accessor::asFlash(images::play), 10);
ui::ImageButton stopButton(210, 140, modm::accessor::asFlash(images::stop), 10);

uint16_t rpmSetting = 1000;
MotorControl motorControl;
bool motorEnable = false;

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
        settingNumeric.setValue(rpmSetting);
    });

    downButton.registerClick([]() {
        uint16_t increment = std::pow(10, 3 - settingNumeric.getActiveDigit());
        rpmSetting -= increment;
        settingNumeric.setValue(rpmSetting);
    });

    playButton.registerClick([]() {
        playButton.hide();
        stopButton.show();
        motorEnable = true;
    });

    stopButton.registerClick([]() {
        stopButton.hide();
        playButton.show();
        motorEnable = false;
    });
}

namespace touchCalibration {
    const uint16_t MinX = 390;
    const uint16_t MinY = 360;
    const uint16_t MaxX = 3880;
    const uint16_t MaxY = 3800;
};

using freqCounter = AnalogFrequencyCounter<Timer2, Board::SystemClock>;

#ifdef PWM_ESC_CONTROL
void setupPwm() {
    motor::Pin::setOutput(true);
    motor::Timer::enable();
    motor::ConnectType::connect();

    motor::Timer::setMode(
        motor::Timer::Mode::UpCounter,
        motor::Timer::SlaveMode::Disabled
    );
    motor::Timer::setPeriod<Board::SystemClock>(20 * 1000); // microseconds
    motor::Timer::configureOutputChannel(
        motor::Chan,
        motor::Timer::OutputCompareMode::Pwm,
        motor::Timer::PinState::Enable,
        motor::Timer::OutputComparePolarity::ActiveHigh,
        motor::Timer::PinState::Disable
    );
    motor::Timer::enableOutput();
    motor::Timer::start();

    //motor::Timer::setNormalPwm(motor::Chan);
}

void setPulseWidth(uint32_t width_us) {
    // modm advanced timer (i.e. timer1) doesn't have getTickFrequency, so hacking
    // some local assumptions here instead
    float cycles = Board::SystemClock::Timer1 / (TIM1->PSC + 1) * (float)width_us / 1e6f;
    //float cycles = (float)motor::Timer::getTickFrequency<Board::SystemClock>() * (float)width_us / 1e6f;

    motor::Timer::setCompareValue(motor::Chan, (uint16_t)(cycles + 0.5));
    motor::Timer::applyAndReset();
}

#endif

MODM_ISR(TIM2)
{
    freqCounter::isrHandler();
}

/** Send RPM command to an STSPIN motor controller via UART */
template<class UsartType>
void send_rpm_command(uint32_t rpm) {
    // Scale RPM to the units used by the motor controller, which are
    // electrical rev/s times 100
    // 7 electrical revs per mechanical rev (property of motor)
    // 60 RPM per RPS
    // 100 scale factor for units
    uint32_t scaled_electrical_rps = rpm * 7 * 100 / 60;

    // Really hacky message serializer lives here.
    uint8_t buf[11];
    // Sync bytes
    buf[0] = 2;
    buf[1] = 3;
    // ID
    buf[2] = 0;
    buf[3] = 0;
    // length
    buf[4] = 4;
    // Data
    *((uint32_t *)&buf[5]) = scaled_electrical_rps;
    // checksum
    buf[9] = 0;
    buf[10] = 0;
    for(uint32_t i=0; i<9; i++) {
        buf[9] += buf[i];
        buf[10] += buf[9];
    }
    UsartType::write(buf, sizeof(buf));
}

int main() {
    Board::initialize();

#ifdef PWM_ESC_CONTROL
    setupPwm();
    setPulseWidth(800);
#else
    //motor::Uart::connect<GpioA9::Tx>();
    motor::Uart::initialize<Board::SystemClock, 9600>();
    //motor::Pin::setOutput(true);
    motor::ConnectType::connect();
#endif
    freqCounter::initialize();

    display::Spi::connect<display::Sck::Sck, display::Miso::Miso, display::Mosi::Mosi>();
	display::Spi::initialize<Board::SystemClock, 2248_kHz, 20_pct>();
	tft.initialize();
    tft.enableBacklight(true);
    tft.setRotation(modm::ili9341::Rotation::Rotate90);

    touchpins::Cs::setOutput(true);
    touch.initialize();

	Board::LedUser::set();

	tft.setColor(modm::glcd::Color::red());
    tft.setBackgroundColor(modm::glcd::Color::white());
    tft.clear();
	int16_t w = tft.getWidth();
	int16_t h = tft.getHeight();

    BuildUi();
    uiManager.redraw();
    settingNumeric.setValue(rpmSetting);
    settingNumeric.setActiveDigit(1);

    while(true) {
        freqCounter::task();

        if(touchTimer.execute()) {
            modm::glcd::Point p;
            bool touch_active = touch.read(&p);

            int16_t px = w - (p.x - touchCalibration::MinX) * w / (touchCalibration::MaxX - touchCalibration::MinX);
            int16_t py = h - (p.y - touchCalibration::MinY) * h / (touchCalibration::MaxY - touchCalibration::MinY);
            uiManager.handleTouchStatus(touch_active, px, py);
        }

        if(motorTimer.execute()) {
            uint32_t rpm = (uint32_t)(60 * freqCounter::getFrequency());

#ifdef PWM_ESC_CONTROL
            if(motorEnable) {
                motorControl.set_speed(rpmSetting);
            } else {
                motorControl.set_speed(0);
            }
            float pwm = motorControl.update((float)rpm);
            setPulseWidth((uint32_t)pwm);
#else

            if(motorEnable) {
                send_rpm_command<motor::Uart>(rpmSetting);
            } else {
                send_rpm_command<motor::Uart>(0);
            }
#endif
            actualNumeric.setValue(rpm);
        }
    }
}