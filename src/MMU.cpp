#include "MMU.h"
#include "Timer.h"
#include <fstream>
#include <fmt/core.h>
#include <cstring>
#include <iostream>


namespace emulator
{
    MMU::MMU() :
        boot_rom(std::make_unique<std::array<uint8_t, 256>>()),
        rom_bank_00(std::make_unique<std::array<uint8_t, 0x4000>>()),
        rom_bank_01(std::make_unique<std::array<uint8_t, 0x4000>>()),
        vram(std::make_unique<std::array<uint8_t, 0x2000>>()),
        ext_ram(std::make_unique<std::array<uint8_t, 0x2000>>()),
        wram(std::make_unique<std::array<uint8_t, 0x2000>>()),
        oam(std::make_unique<std::array<uint8_t, 0x00A0>>()),
        io_reg(std::make_unique<std::array<uint8_t, 0x0080>>()),
        hram(std::make_unique<std::array<uint8_t, 0x007F>>()),
        ie_reg(0xE0),
        internal_div(0),
        m_dma_bus_conflict(false),
        m_boot_rom_enabled(true),
        m_STAT((STAT_REG&)io_reg->at(STAT))
    {
        boot_rom->fill(0x00);
        rom_bank_00->fill(0xFF);
        rom_bank_01->fill(0xFF);
        vram->fill(0);
        ext_ram->fill(0);
        wram->fill(0);
        oam->fill(0);
        io_reg->fill(0);
        hram->fill(0);

        io_reg->at(P1_JOYP) = 0xFF;
        timer = nullptr;
    }

    uint8_t MMU::read(uint16_t adr)
    {
        auto p = get_host_adr(adr);

        return (p == nullptr) ? 0xFF : *p;
    }

    void  MMU::write(uint16_t adr, uint8_t v)
    {
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
                internal_div = 0;
                break;
            case SB:
                if (DEBUG_LOG_ENABLED)
                {
                    std::cout << fmt::format("{:c}", v);
                }
                break;
            case DMA:
                timer->add_timer(4, std::bind(&MMU::oam_dma_transfer, this, v));
                break;
            case BANK:
                if (m_boot_rom_enabled && v == 1)
                {
                    m_boot_rom_enabled = false;
                }
                break;
            case LYC:
                m_STAT.coincidence_flag = (io_reg->at(LY) == io_reg->at(LYC));
                break;
            }
        }
    }

    uint8_t* MMU::get_host_adr(uint16_t gb_adr)
    {
        if (0x0000 <= gb_adr && gb_adr <= 0x3FFF)
        {
            if (gb_adr < 0x0100 && m_boot_rom_enabled)
            {
                return boot_rom->data() + gb_adr;
            }

            return rom_bank_00->data() + gb_adr;
        }
        else if (0x4000 <= gb_adr && gb_adr <= 0x7FFF)
        {
            return rom_bank_01->data() + gb_adr - 0x4000;
        }
        else if (0x8000 <= gb_adr && gb_adr <= 0x9FFF)
        {
            return vram->data() + gb_adr - 0x8000;
        }
        else if (0xA000 <= gb_adr && gb_adr <= 0xBFFF)
        {
            return ext_ram->data() + gb_adr - 0xA000;
        }
        else if (0xC000 <= gb_adr && gb_adr <= 0xFDFF)
        {
            return wram->data() + (gb_adr & 0x1FFF);
        }
        else if (0xFE00 <= gb_adr && gb_adr <= 0xFE9F)
        {
            return oam->data() + gb_adr - 0xFE00;
        }
        else if (0xFF00 <= gb_adr && gb_adr <= 0xFF7F)
        {
            return io_reg->data() + gb_adr - 0xFF00;
        }
        else if (0xFF80 <= gb_adr && gb_adr <= 0xFFFE)
        {
            return hram->data() + gb_adr - 0xFF80;
        }
        else if (gb_adr == 0xFFFF)
        {
            return &ie_reg;
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

        input.read(reinterpret_cast<char*>(boot_rom.get()), boot_rom->size());
    }

    void  MMU::load_game_rom(std::string path)
    {
        std::ifstream input(path, std::ios::binary);
        // _rom_gb = std::vector<uint8_t>((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

        input.read(reinterpret_cast<char*>(rom_bank_00.get()), rom_bank_00->size());
        input.read(reinterpret_cast<char*>(rom_bank_01.get()), rom_bank_01->size());
    }

    // TO DO : impl DMA Bus Conflicts
    void MMU::oam_dma_transfer(uint8_t src)
    {
        std::memcpy(oam.get(), get_host_adr(src << 8), 0xA0);
    }

    bool MMU::is_locked(uint16_t gb_adr)
    {
        //  HRAM is not affected by DMA transfers
        return (gb_adr < 0x4000) // ROM 
            || (0x8000 <= gb_adr && gb_adr <= 0x9FFF && (m_STAT.ppu_mode > 2)) // VRAM
            || (0xFE00 <= gb_adr && gb_adr <= 0xFE9F && (m_STAT.ppu_mode > 1)); // OAM
    }
}

