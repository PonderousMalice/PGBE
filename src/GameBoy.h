#pragma once
#include "MMU.h"
#include "PPU.h"
#include "SM83.h"
#include "Timer.h"
#include <fstream>
#include <memory>
#include <string_view>

namespace emulator
{
    class GameBoy
    {
    public:
        GameBoy();
        GameBoy(std::string_view rom_path);

        void init();
        void update();
        color get_color(int x, int y);
        void reset_ppu();
    private:
        std::unique_ptr<MMU> _mmu;
        std::unique_ptr<PPU> _ppu;
        std::unique_ptr<Timer> _timer;
        std::unique_ptr<SM83> _cpu;
        std::ofstream _logs;
        std::string _rom_path;
    };
}