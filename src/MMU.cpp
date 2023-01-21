#include "MMU.h"
#include "Timer.h"
#include <fstream>
#include <fmt/core.h>
#include <cstring>
#include <iostream>


namespace emulator
{
    MMU::MMU() :
        BOOT_ROM(std::make_unique<std::array<uint8_t, 256>>()),
        ROM_BANK_00(std::make_unique<std::array<uint8_t, 0x4000>>()),
        ROM_BANK_01(std::make_unique<std::array<uint8_t, 0x4000>>()),
        VRAM(std::make_unique<std::array<uint8_t, 0x2000>>()),
        EXT_RAM(std::make_unique<std::array<uint8_t, 0x2000>>()),
        WRAM(std::make_unique<std::array<uint8_t, 0x2000>>()),
        OAM(std::make_unique<std::array<uint8_t, 0x00A0>>()),
        IO_REG(std::make_unique<std::array<uint8_t, 0x0080>>()),
        HRAM(std::make_unique<std::array<uint8_t, 0x007F>>()),
        IE_REG(0),
        INTERNAL_DIV(0)
    {
        BOOT_ROM->fill(0x00);
        ROM_BANK_00->fill(0xFF);
        ROM_BANK_01->fill(0xFF);
        VRAM->fill(0);
        EXT_RAM->fill(0);
        WRAM->fill(0);
        OAM->fill(0);
        IO_REG->fill(0);
        HRAM->fill(0);
        _dma_bus_conflict = false;
        _boot_rom_enabled = true;
        timer = nullptr;
    }

    uint8_t MMU::read(uint16_t adr)
    {
        auto p = get_host_adr(adr);

        return (p == nullptr) ? 0xFF : *p;
    }

    void  MMU::write(uint16_t adr, uint8_t v)
    {
        //  HRAM is not affected by DMA transfers
        if (is_locked(adr))
        {
            return;
        }

        auto p = get_host_adr(adr);
        if (p != nullptr)
        {
            *p = v;
        }

        if (0xFF00 <= adr && adr <= 0xFF7F)
        {
            switch (adr & 0xFF)
            {
            case DIV:
                INTERNAL_DIV = 0;
                break;
            case SB:
                std::cout << fmt::format("{:c}", v);
                break;
            case DMA:
                // DMA TRANSFER
                // start transfer in 4 cycles
                break;
            case BANK:
                if (_boot_rom_enabled && v == 0)
                {
                    _boot_rom_enabled = false;
                }
                break;
            }
        }
    }

    uint8_t* MMU::get_host_adr(uint16_t gb_adr)
    {
        if (0x0000 <= gb_adr && gb_adr <= 0x3FFF)
        {
            if (gb_adr < 0x0100 && _boot_rom_enabled)
            {
                return BOOT_ROM->data() + gb_adr;
            }

            return ROM_BANK_00->data() + gb_adr;
        }
        else if (0x4000 <= gb_adr && gb_adr <= 0x7FFF)
        {
            return ROM_BANK_01->data() + gb_adr - 0x4000;
        }
        else if (0x8000 <= gb_adr && gb_adr <= 0x9FFF)
        {
            return VRAM->data() + gb_adr - 0x8000;
        }
        else if (0xA000 <= gb_adr && gb_adr <= 0xBFFF)
        {
            return EXT_RAM->data() + gb_adr - 0xA000;
        }
        else if (0xC000 <= gb_adr && gb_adr <= 0xFDFF)
        {
            return WRAM->data() + (gb_adr & 0x1FFF);
        }
        else if (0xFE00 <= gb_adr && gb_adr <= 0xFE9F)
        {
            return OAM->data() + gb_adr - 0xFE00;
        }
        else if (0xFF00 <= gb_adr && gb_adr <= 0xFF7F)
        {
            return IO_REG->data() + gb_adr - 0xFF00;
        }
        else if (0xFF80 <= gb_adr && gb_adr <= 0xFFFE)
        {
            return HRAM->data() + gb_adr - 0xFF80;
        }
        else if (gb_adr == 0xFFFF)
        {
            return &IE_REG;
        }

        return nullptr;
    }

    void  MMU::load_boot_rom(std::string path)
    {
        std::ifstream input(path, std::ios::binary);

        if (!input)
        {
            fmt::print("Missing boot rom file...");
            exit(1);
        }

        input.read(reinterpret_cast<char*>(BOOT_ROM.get()), BOOT_ROM->size());
    }

    void  MMU::load_game_rom(std::string path)
    {
        std::ifstream input(path, std::ios::binary);
        // _rom_gb = std::vector<uint8_t>((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

        input.read(reinterpret_cast<char*>(ROM_BANK_00.get()), ROM_BANK_00->size());
        input.read(reinterpret_cast<char*>(ROM_BANK_01.get()), ROM_BANK_01->size());
    }

    void MMU::oam_dma_transfer(uint8_t src)
    {
        std::memcpy(OAM.get(), get_host_adr(src << 8), 0xA0);
    }

    bool MMU::is_locked(uint16_t gb_adr)
    {
        const STAT_REG& stat = (STAT_REG&)IO_REG->at(STAT);

        return (gb_adr < 0x4000) // ROM 
            || (0x8000 <= gb_adr && gb_adr <= 0x9FFF && (stat.flags.ppu_mode > 2)) // VRAM
            || (0xFE00 <= gb_adr && gb_adr <= 0xFE9F && (stat.flags.ppu_mode > 1)) // OAM
            || !(0xFF00 <= gb_adr && gb_adr <= 0xFFFE) && _dma_bus_conflict;
    }
}

