#ifndef N64_GAMECAT_WIRE_H
#define N64_GAMECAT_WIRE_H


#include <cstdint>
#include <vector>

class Wire {
public:
    void SetTimings(std::vector<uint32_t> _timings);

    void Tick();
    void Tick(uint32_t num);

    uint8_t GetState();

    void Reset();

private:
    std::vector<uint32_t> timings;
    uint8_t state = 1;
    uint32_t time = 0;
    uint32_t index = 0;
};


#endif //N64_GAMECAT_WIRE_H
