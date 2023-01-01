#include "MMU.h"
#include <fstream>
#include <fmt/core.h>

namespace emulator
{
    uint8_t MMU::read(uint16_t adr)
    {
        return get_host_adr(adr);
    }

    void MMU::write(uint16_t adr, uint8_t v)
    {
        if (adr > 0x7FFF)
        {
        uint8_t& p = get_host_adr(adr);

        if (&p != &_null)
        {
            p = v;
        }
    }

        if (adr == 0xFF01)
        {
            fmt::print("{:c}", v);
        }
    }

    uint8_t& MMU::get_host_adr(uint16_t gb_adr)
    {
        if (gb_adr <= 0x3FFF)
        {
            if (gb_adr < 0x0100 && boot_rom_enabled())
        {
                return _boot_rom->at(gb_adr);
        }

            return _rom_bank_00->at(gb_adr);
        }
        else if (gb_adr > 0x3FFF && gb_adr <= 0x7FFF)
        {
            return _rom_bank_01->at(gb_adr - 0x4000);
        }
        else if (gb_adr > 0x7FFF && gb_adr <= 0x9FFF)
        {
            return _vram->at(gb_adr - 0x8000);
        }
        else if (gb_adr > 0x9FFF && gb_adr <= 0xBFFF && _external_ram)
        {
            return _external_ram->at(gb_adr - 0xA000);
        }
        else if (gb_adr > 0xBFFF && gb_adr <= 0xDFFF)
        {
            return _wram->at(gb_adr - 0xC000);
        }
        else if (gb_adr > 0xDFFF && gb_adr <= 0xFDFF)
        {
            return _wram->at(gb_adr - 0xE000);
        }
        else if (gb_adr > 0xFDFF && gb_adr <= 0xFE9F)
        {
            return OAM->at(gb_adr - 0xFE00);
        }
        else if (gb_adr > 0xFEFF && gb_adr <= 0xFF7F)
        {
            return _io_reg->at(gb_adr - 0xFF00);
        }
        else if (gb_adr > 0xFF7F && gb_adr <= 0xFFFE)
        {
            return _hram->at(gb_adr - 0xFF80);
        }
        else if (gb_adr == 0xFFFF)
        {
            return _ie_reg;
        }

        return _null;
    }

    void MMU::load_boot_rom(std::string path)
    {
        std::ifstream input(path, std::ios::binary);

        if (!input)
        {
            fmt::print("Missing boot rom file...");
            exit(1);
        }

        input.read(reinterpret_cast<char*>(_boot_rom.get()), _boot_rom->size());
    }

    void MMU::load_game_rom(std::string path)
    {
        std::ifstream input(path, std::ios::binary);
       // _rom_gb = std::vector<uint8_t>((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

        input.read(reinterpret_cast<char*>(_rom_bank_00.get()), _rom_bank_00->size());
        input.read(reinterpret_cast<char*>(_rom_bank_01.get()), _rom_bank_01->size());
    }
}

