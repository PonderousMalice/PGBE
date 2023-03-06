#include "Timer.h"
#include "utils.h"

namespace PGBE
{
    Timer::Timer(MMU* mmu, PPU* ppu) :
        m_ppu(ppu),
        m_mmu(mmu),
        m_div(mmu->io_reg->at(DIV)),
        m_tima(mmu->io_reg->at(TIMA)),
        m_tma(mmu->io_reg->at(TMA)),
        m_tac(mmu->io_reg->at(TAC)),
        m_internal_div(mmu->internal_div),
        m_IF(mmu->io_reg->at(IF)),
        m_timers()
    {
        m_div = 0;
        m_internal_div = 0;
        m_prev_and_res = false;
        m_prev_tima = 0;
    }

    void Timer::schedule_task(int delay, std::function<void()> callback)
    {
        Task t
        {
            .count = 0,
            .delay = delay,
            .callback = callback
        };

        m_timers.push_back(t);
    }

    void Timer::m_update_clock()
    {
        const std::array<int, 4> _bit_pos
        {
            9,
            3,
            5,
            7
        };

        m_div = (((++m_internal_div) & 0xFF00) >> 8);

        // TIMA is fucked up dude
        int pos = _bit_pos.at(m_tac & 0b0011);
        bool div_bit = (m_internal_div >> pos) & 0b0001;
        bool and_res = div_bit && m_timer_enabled();
        if (!and_res && m_prev_and_res)
        {
            m_prev_tima = m_tima++;
        }

        m_prev_and_res = and_res;
        if (m_prev_tima == 0xFF && m_tima == 0x00)
        {
            schedule_task(4, std::bind(&Timer::m_tima_overflow, this));
        }
    }

    void Timer::advance_cycle()
    {
        for (int i = 0; i < 4; ++i)
        {
            m_update_clock();
            m_check_timers();
            m_ppu->tick();
        }
    }

    bool Timer::m_timer_enabled()
    {
        return ((m_tac >> 2) & 0x01) > 0;
    }

    void Timer::m_check_timers()
    {
        for (auto it = m_timers.begin(); it != m_timers.end();)
        {
            if (it->count++ >= it->delay)
            {
                it->callback();
                it = m_timers.erase(it);
            }
            else
            {
                it++;
            }
        }
    }

    void Timer::m_tima_overflow()
    {
        set_bit(m_IF, 2);
        m_tima = m_tma;
    }
}