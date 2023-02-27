#pragma once
#include "defines.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace emulator
{
    inline bool is_set_bit(u16 i, int n)
    {
        return (i >> n) & 1U;
    }

    inline void clear_bit(u8& i, int n)
    {
        i &= ~(1U << n);
    }

    inline void set_bit(u8& i, int n, bool value = true)
    {
        i = i & ~(1U << n) | (value << n);
    }

    inline void toggle_bit(u8& i, int n)
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

    inline void nsleep(u64 nanoseconds)
    {
#ifdef _WIN32
        HANDLE timer = nullptr;
        LARGE_INTEGER sleepTime;

        sleepTime.QuadPart = -(nanoseconds / 100LL);

        timer = CreateWaitableTimer(nullptr, true, nullptr);
        SetWaitableTimer(timer, &sleepTime, 0, nullptr, nullptr, false);
        if (timer == nullptr)
        {
            exit(1);
        }
        if (WaitForSingleObject(timer, INFINITE) != WAIT_OBJECT_0)
        {
            exit(1);
        }
        CloseHandle(timer);
#endif
    }
}