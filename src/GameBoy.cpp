#include "GameBoy.h"
#include "utils.h"

namespace PGBE
{
    GameBoy::GameBoy() :
        mmu(),
        ppu(&mmu),
        timer(&mmu, &ppu),
        cpu(&mmu, &timer),
        show_perf(true),
        show_memory(false),
        show_vram(false),
        show_main_menu_bar(false)
    {
        mmu.timer = &timer;
    }

    GameBoy::~GameBoy()
    {
    }

    void GameBoy::use_button(const GB_BUTTON b, const bool pressed)
    {
        if (pressed)
        {
            u8 &r_IF = mmu.io_reg->at(IF);

            set_bit(r_IF, 4);
        }

        mmu.p_input.at(b) = pressed;
    }
}