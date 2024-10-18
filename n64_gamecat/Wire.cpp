#include "Wire.h"

#include <utility>

void Wire::SetTimings(std::vector<uint32_t> _timings) {
    this->timings = std::move(_timings);
}

uint8_t Wire::GetState() {
    return state;
}

void Wire::Tick() {
    if (time >= timings[index] && index < timings.size())
    {
        state = !state;
        ++index;
    }
    ++time;
}

void Wire::Reset() {
    state = 1;
    time = 0;
    index = 0;
}

void Wire::Tick(uint32_t num) {
    time += num;

    for (int i = 0; i < num; ++i) {
        Tick();
    }
}
