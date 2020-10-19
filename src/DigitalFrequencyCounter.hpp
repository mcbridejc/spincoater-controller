#pragma once

#include <modm/board.hpp>
#include "MovingAverage.hpp"

template<typename Timer, uint8_t Chan>
class DigitalFrequencyCounter {

    static void intialize() {
        Timer::enable();
        Timer::setMode(
            Timer::Mode::UpCounter,
            Timer::SlaveMode::Disabled
        );
        Timer::configureInputChannel(
            Chan,
            Timer::InputCaptureMapping::InputOwn,
            Timer::InputCapturePrescaler::Div1,
            Timer::InputCapturePolarity::Rising,
            8
        );
        Timer::enableInterruptVector(true, 3);
        if(Chan == 1) {
            Timer::enableInterrupt(Timer::Interrupt::CaptureCompare1);
        } else if(Chan == 2) {
            Timer::enableInterrupt(Timer::Interrupt::CaptureCompare2);
        } else if(Chan == 3) {
            Timer::enableInterrupt(Timer::Interrupt::CaptureCompare3);
        } else if (Chan == 4) {
            Timer::enableInterrupt(Timer::Interrupt::CaptureCompare4);
        }
        Timer::enableInterrupt(Timer::Interrupt::Update);
        Timer::start();
    }

    static uint32_t getFrequency() {
        if(freqValid) {
            uint32_t tickFreq = Timer::template getTickFrequency<Board::SystemClock>();
            uint32_t period = periodFilter.get();
            if(period > 0) {
                // 7 electrical rev / mechanical rev
                return (uint32_t)((float)60.0 * tickFreq / period / 7);
            } else {
                return 0;
            }
        } else {
            return 0;
        }
    }

    // Application needs to create ISR and call this handler
    static void IsrHandler()
    {
        static uint32_t highCount; // Count overflows
        static uint32_t lastCapture;
        auto flags = Timer::getInterruptFlags();
        Timer::acknowledgeInterruptFlags(flags);

        uint32_t newCount = Timer::getCompareValue(Chan) + highCount * 65536;

        if(flags.value & (uint32_t)Timer::InterruptFlag::Update) {
            highCount += 1;
            if(newCount - lastCapture > 500000000) {
                freqValid = false;
            }
        }

        if(flags.value & (uint32_t)Timer::InterruptFlag::CaptureCompare1) {
            periodFilter.push(newCount - lastCapture);
            lastCapture = newCount;
            freqValid = true;
        }
    }

    static bool freqValid;
    static MovingAverage<20> periodFilter;
};
