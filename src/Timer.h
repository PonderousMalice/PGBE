#pragma once
#include <functional>
#include "MMU.h"
#include "PPU.h"

namespace emulator
{
    // task ? promise ? Waiter ?
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
        
        // schedule task ?
        void add_timer(int duration, std::function<void()> callback);
        void advance_cycle();
    private:
        
        void m_update_clock();
        void m_check_timers();
        bool m_timer_enabled();

        void m_tima_overflow();

        MMU* m_mmu;
        PPU* m_ppu;
        uint16_t& m_internal_div;
        uint8_t& m_div, &m_tima, &m_tma, &m_tac, &m_IF;
        bool m_prev_and_res;
        uint8_t m_prev_tima;
        std::vector<ClockWatch> m_timers;
    };
}