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
        Timer(MMU* mmu, PPU* ppu);

        void advance_cycle();
    private:
        void add_timer(int duration, std::function<void()> callback);
        void update_clock();
        void check_timers();
        bool timer_enabled();

        void tima_overflow();

        MMU* _mmu;
        PPU* _ppu;
        uint16_t& _internal_div;
        uint8_t& _div, &_tima, &_tma, &_tac, &_interupt_flag;
        bool _prev_and_res;
        uint8_t _prev_tima;
        std::vector<ClockWatch> timers;
    };
}