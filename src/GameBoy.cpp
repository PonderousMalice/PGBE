#include "const.h"
#include "GameBoy.h"
#include <fmt/core.h>
#include <iostream>

namespace emulator
{
    GameBoy::GameBoy() :
        GameBoy("")
    {}

    GameBoy::GameBoy(std::string_view rom_path) :
        _rom_path(rom_path),
        _logs(log_file_path),
        _mmu(std::make_unique<MMU>()),
        _ppu(std::make_unique<PPU>(
            (LCD_C&)_mmu->IO_REG->at(LCDC),
            (STAT_REG&)_mmu->IO_REG->at(STAT),
            _mmu->IO_REG->at(LY),
            _mmu->IO_REG->at(SCX),
            _mmu->IO_REG->at(SCY),
            std::bit_cast<std::array<sprite_attributes, 40>*>(_mmu->OAM.get()),
            std::bit_cast<std::array<uint8_t, 0x2000>*>(_mmu->VRAM.get()))),
        _timer(std::make_unique<Timer>(_mmu.get(), _ppu.get())),
        _cpu(std::make_unique<SM83>(_mmu.get(), _timer.get()))
    {
        _mmu->timer = _timer.get();
    }

    void GameBoy::update()
    {
        while(!_ppu->frame_completed())
        {
            _cpu->run();
        }
    }

    color GameBoy::get_color(int x, int y)
    {
        return _ppu->get_color(x, y);
    }

    void GameBoy::reset_ppu()
    {
        _ppu->reset();
    }

    void GameBoy::init()
    {
        _mmu->load_boot_rom(boot_rom_path);
        if (!_rom_path.empty())
        {
            _mmu->load_game_rom(_rom_path);
        }
    }
}

