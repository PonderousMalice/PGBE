#include "Timer.h"

namespace emulator
{
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

    void Timer::advance_cycle()
    {
        update_clock();
        _ppu->tick();
    }
}