#pragma once
#include <functional>
#include "MMU.h"
#include "PPU.h"

namespace emulator
{
    struct ClockWatch
    {
        int count;
        int duration;
        std::function<void()> callback;
    };

    class Timer
    {
    public:
        Timer(MMU* mmu, PPU* ppu)
        {
            _mmu = mmu;
            _ppu = ppu;
        }

        void advance_cycle();
    private:
        void add_timer(int duration, std::function<void()> callback);
        void update_clock();

        std::vector<ClockWatch> timers;
        MMU* _mmu;
        PPU* _ppu;
    };
}