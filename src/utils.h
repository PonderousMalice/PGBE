#pragma once
#include "integers.h"

inline bool is_set_bit(u16 i, int n)
{
    return (i >> n) & 1U;
}

inline void clear_bit(u8 &i, int n)
{
    i &= ~(1U << n);
}

inline void set_bit(u8 &i, int n, bool value = true)
{
    i = i & ~(1U << n) | (value << n);
}

inline void toggle_bit(u8 &i, int n)
{
    i ^= ~(1U << n);
}

inline u16 combine(u8 lsb, u8 msb)
{
    return ((u16)msb << 8) | lsb;
}

inline u8 LSB(u16 i)
{
    return (u8)(i & 0x00FF);
}

inline u8 MSB(u16 i)
{
    return (u8)((i & 0xFF00) >> 8);
}