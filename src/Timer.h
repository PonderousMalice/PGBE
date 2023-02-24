#pragma once
#include "MMU.h"
#include "PPU.h"
#include <functional>

namespace emulator
{
    struct Task
    {
        int count;
        int delay;
        std::function<void()> callback;
    };
    
    class Timer
    {
    public:
        Timer(MMU* mmu, PPU* ppu);
        
        void schedule_task(int delay, std::function<void()> callback);
        void advance_cycle();
    private:
        void m_update_clock();
        void m_check_timers();
        bool m_timer_enabled();

        void m_tima_overflow();

        MMU* m_mmu;
        PPU* m_ppu;
        u16& m_internal_div;
        u8& m_div, &m_tima, &m_tma, &m_tac, &m_IF;
        bool m_prev_and_res;
        u8 m_prev_tima;
        std::vector<Task> m_timers;
    };
}