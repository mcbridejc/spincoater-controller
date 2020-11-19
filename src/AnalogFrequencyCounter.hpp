#include <modm/platform.hpp>
#include <modm/board.hpp>

using Adc = modm::platform::Adc1;

// Size of buffer to collect samples from ISR
static const uint32_t SampleBufferSize = 256;
// Fixed point scale for calculations 
// sampleScale * meanScale must be < 2**20
static constexpr uint32_t SampleScale = 16;
// Fixed point scale for filter coefficients
static constexpr uint32_t MeanScale = 32768;
// Number of periods in moving average
static const uint32_t NumPeriods = 6;
// Filter coefficient for IIR mean filter
static constexpr uint32_t MeanFilterAlpha = (0.001 * MeanScale);
// ADC sampling period
static const uint32_t SamplePeriodUs = 1000;
// Hysteresis threshold. Input must reach `runningMean +/- MinSpread` to trigger a 
// transition
static const uint8_t MinSpread = 150;
// If no edges are measured in this time, output goes to zero
static const uint32_t TimeoutMs = 2000;

static uint16_t sampleBuffer[SampleBufferSize];
static uint32_t sampleHead;
static uint32_t sampleTail;
static uint32_t samplesSinceLastEdge = 0;
static uint32_t runningMean = 2048 * SampleScale;
static bool lastState = false;
static uint32_t periods[NumPeriods] = {0};
static uint32_t periodInPtr = 0;
static float periodInSeconds = 0.0f;

MODM_ISR(ADC1_2) {
    Adc::acknowledgeInterruptFlag(Adc::getInterruptFlags());

    uint16_t sample = Adc::getValue();
    // Convert 12-bit sample to 8 bit
    sampleBuffer[sampleHead] = (sample);
    sampleHead = (sampleHead + 1) % SampleBufferSize;\
}

template<class Timer, class SystemClock>
class AnalogFrequencyCounter {
public:
    static void initialize() {
        Timer::enable();
        Timer::template setPeriod < SystemClock >(SamplePeriodUs);
        Timer::setMode(
            Timer::Mode::UpCounter,
            Timer::SlaveMode::Disabled
        );
        // TODO: This works with basic timers. Advanced timers need a special 
        // special case here
        Timer::enableInterruptVector(true, 4);
        Timer::enableInterrupt(Timer::Interrupt::Update);
        Timer::start();

        Adc::initialize();
        Adc::connect<GpioA0::In1>();
        Adc::setChannel(Adc::Channel::Channel1, Adc::SampleTime::Cycles248);
        Adc::enableInterruptVector(4);
        Adc::enableInterrupt(Adc::Interrupt::EndOfRegularConversion);
    }

    static void task() {
        while(sampleTail != sampleHead) {
            processSample(sampleBuffer[sampleTail]);
            sampleTail = (sampleTail + 1) % SampleBufferSize;
        }

        uint32_t periodAvg = 0;
        for(uint32_t i=0; i<NumPeriods; i++) {
            periodAvg += periods[i];
        }
        periodInSeconds = (float)periodAvg / NumPeriods * (float)SamplePeriodUs / 1e6;
    }

    // Application needs to create ISR and call this handler
    static inline void isrHandler() {
        Timer::acknowledgeInterruptFlags(Timer::getInterruptFlags());
        Adc::startConversion();
    }

    static inline void processSample(uint16_t sample) {
        sample *= SampleScale;
        runningMean = (runningMean * (MeanScale - MeanFilterAlpha) + sample * MeanFilterAlpha + MeanScale / 2) / MeanScale;
        samplesSinceLastEdge++;
        if(!lastState && sample > runningMean + MinSpread * SampleScale) {
            lastState = true;
            periods[periodInPtr] = samplesSinceLastEdge;
            periodInPtr = (periodInPtr + 1) % NumPeriods;
            samplesSinceLastEdge = 0;
        } else if (lastState && sample < runningMean - MinSpread * SampleScale) {
            lastState = false;
        }
    }

    static float getFrequency() {
        static constexpr uint32_t timeoutSamples = TimeoutMs * 1000 / SamplePeriodUs;
        if(periodInSeconds > 0.0 && samplesSinceLastEdge < timeoutSamples) {
            return 1.0f / periodInSeconds;
        } else {
            return 0.0f;
        }
    }
};
