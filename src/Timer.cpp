#include "Timer.h"
#include "utils.h"

namespace emulator
{
    Timer::Timer(MMU* mmu, PPU* ppu) :
        _ppu(ppu),
        _mmu(mmu),
        _div(mmu->IO_REG->at(DIV)),
        _tima(mmu->IO_REG->at(TIMA)),
        _tma(mmu->IO_REG->at(TMA)),
        _tac(mmu->IO_REG->at(TAC)),
        _internal_div(mmu->INTERNAL_DIV),
        _interupt_flag(mmu->IO_REG->at(IF))
    {
        _div = 0;
        _internal_div = 0;
        _prev_and_res = false;
        _prev_tima = 0;
    }


    void Timer::add_timer(int duration, std::function<void()> callback)
    {
        ClockWatch t
        {
            .count = 0,
            .duration = duration,
            .callback = callback
        };

        timers.push_back(t);
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

        _div = (((++_internal_div) & 0xFF00) >> 8);

        // TIMA is fucked up dude
        int pos = _bit_pos.at(_tac & 0b0011);
        bool div_bit = (_internal_div >> pos) & 0b0001;
        bool and_res = div_bit && timer_enabled();
        if (!and_res && _prev_and_res)
        {
            ++_tima;
        }

        _prev_and_res = and_res;
        if (_prev_tima == 0xFF && _tima == 0x00)
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
            _ppu->tick();
        }
    }

    bool Timer::timer_enabled()
    {
        return (_tac >> 2) & 0x01;
    }

    void Timer::check_timers()
    {
        for (auto it = timers.begin(); it != timers.end();)
        {
            if (it->count++ >= it->duration)
            {
                it->callback();
                timers.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void Timer::tima_overflow()
    {
        set_bit(_interupt_flag, 2);
        _tima = _tma;
    }
}