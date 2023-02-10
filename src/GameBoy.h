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
    enum GB_BUTTON
    {
        GB_UP = 2,
        GB_DOWN = 3,
        GB_LEFT = 1,
        GB_RIGHT = 0,
        GB_A = 4,
        GB_B = 5,
        GB_START = 7,
        GB_SELECT = 6
    };

    class GameBoy
    {
    public:
        GameBoy();
        GameBoy(std::string_view rom_path);

        void init();
        void update();
        color get_color(int x, int y);
        void reset_ppu();

        void use_button(GB_BUTTON b, bool pressed);
    private:
        MMU m_mmu;
        PPU m_ppu;
        Timer m_timer;
        SM83 m_cpu;
        std::ofstream m_logs;
        std::string m_rom_path;
    };
}