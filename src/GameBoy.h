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
        GB_UP,
        GB_DOWN,
        GB_LEFT,
        GB_RIGHT,
        GB_A,
        GB_B,
        GB_START,
        GB_SELECT
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

        void use_button(GB_BUTTON b, bool realeased);
    private:
        MMU m_mmu;
        PPU m_ppu;
        Timer m_timer;
        SM83 m_cpu;
        std::ofstream m_logs;
        std::string m_rom_path;
    };
}