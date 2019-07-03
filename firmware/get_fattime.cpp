#include "Particle.h"
#include "fatfs/ff.h"

DWORD get_fattime (void) {
    uint32_t current_time = Time.now();
    DWORD res = 0;
    res |= (Time.year(current_time) & 0x7F) << 25;
    res |= (Time.month(current_time) & 0xF) << 21;
    res |= (Time.day(current_time) & 0x1F) << 16;
    res |= (Time.hour(current_time) & 0x1F) << 11;
    res |= (Time.second(current_time) / 2 & 0x1F) << 0;
    return res;
}
