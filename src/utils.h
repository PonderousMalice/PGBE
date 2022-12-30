#pragma once

namespace emulator
{
    inline bool isSetBit(uint16_t i, int n)
    {
        return (i >> n) & 1U;
    }

    inline void clearBit(uint8_t& i, int n)
    {
        i &= ~(1U << n);
    }

    inline void setBit(uint8_t& i, int n, bool value = true)
    {
        i = i & ~(1U << n) | (value << n);
    }

    inline void toggleBit(uint8_t& i, int n)
    {
        i ^= ~(1U << n);
    }

    inline uint16_t combine(uint8_t lsb, uint8_t msb)
    {
        return ((uint16_t)msb << 8) | lsb;
    }

    inline uint8_t LSB(uint16_t i)
    {
        return (uint8_t)(i & 0x00FF);
    }

    inline uint8_t MSB(uint16_t i)
    {
        return (uint8_t)((i & 0xFF00) >> 8);
    }
}