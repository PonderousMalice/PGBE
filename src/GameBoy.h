#pragma once
#include "integers.h"
#include "MMU.h"
#include "PPU.h"
#include "SM83.h"
#include "Timer.h"
#include <SDL.h>
#include <string_view>

namespace PGBE
{
    constexpr auto BUTTON_START_LINE = __LINE__;
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
    constexpr auto BUTTON_ENUM_SIZE = __LINE__ - BUTTON_START_LINE - 4;

    struct GB_MAP
    {
        GB_BUTTON b;
        SDL_Keycode k;
    };

    struct GameBoy
    {
        MMU mmu;
        PPU ppu;
        Timer timer;
        SM83 cpu;

        bool show_perf;
        bool show_memory;
        bool show_vram;
        bool show_main_menu_bar;

        GameBoy();
        ~GameBoy();

        void use_button(const GB_BUTTON b, const bool pressed);
    };
}