#pragma once

#include <cstdint>

template<uint32_t NTAPS>
class MovingAverage {
public:
    MovingAverage() :
        inPtr(0)
    {
        
    }

    void push(uint32_t value) {
        values[inPtr] = value;
        inPtr = (inPtr + 1) % NTAPS;
    }

    uint32_t get() {
        uint32_t sum = 0;
        for(uint32_t i=0; i<NTAPS; i++) {
            sum += values[i];
        }
        sum /= NTAPS;
        return sum;
    }

private:
    uint32_t values[NTAPS];
    uint32_t inPtr;
};
