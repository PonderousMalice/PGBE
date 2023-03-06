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

    void GameBoy::update()
    {
        // switch to x T-cycles ?
        while (!ppu.frame_completed())
        {
            cpu.run();
        }
    }

    void GameBoy::load_rom(std::string_view path)
    {
        mmu.load_game_rom(path);
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

    color GameBoy::get_color(int x, int y)
    {
        return ppu.get_color(x, y);
    }

    void GameBoy::reset_ppu()
    {
        ppu.reset();
    }
}