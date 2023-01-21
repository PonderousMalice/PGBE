#include "Timer.h"
#include "utils.h"

namespace emulator
{
    Timer::Timer(MMU* mmu, PPU* ppu) :
        m_ppu(ppu),
        m_mmu(mmu),
        m_div(mmu->IO_REG->at(DIV)),
        m_tima(mmu->IO_REG->at(TIMA)),
        m_tma(mmu->IO_REG->at(TMA)),
        m_tac(mmu->IO_REG->at(TAC)),
        m_internal_div(mmu->INTERNAL_DIV),
        m_IF(mmu->IO_REG->at(IF))
    {
        m_div = 0;
        m_internal_div = 0;
        m_prev_and_res = false;
        m_prev_tima = 0;
    }


    void Timer::add_timer(int duration, std::function<void()> callback)
    {
        ClockWatch t
        {
            .count = 0,
            .duration = duration,
            .callback = callback
        };

        m_timers.push_back(t);
    }

    void Timer::update_clock()
    {
        static const std::array<int, 4> _bit_pos
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
        bool and_res = div_bit && timer_enabled();
        if (!and_res && m_prev_and_res)
        {
            ++m_tima;
        }

        m_prev_and_res = and_res;
        if (m_prev_tima == 0xFF && m_tima == 0x00)
        {
            add_timer(4, std::bind(&Timer::tima_overflow, this));
        }
    }

    void Timer::advance_cycle()
    {
        for (int i = 0; i < 4; ++i)
        {
            check_timers();
            update_clock();
            m_ppu->tick();
        }
    }

    bool Timer::timer_enabled()
    {
        return (m_tac >> 2) & 0x01;
    }

    void Timer::check_timers()
    {
        for (auto it = m_timers.begin(); it != m_timers.end();)
        {
            if (it->count++ >= it->duration)
            {
                it->callback();
                m_timers.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void Timer::tima_overflow()
    {
        set_bit(m_IF, 2);
        m_tima = m_tma;
    }
}