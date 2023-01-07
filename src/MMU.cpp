#include "MMU.h"

namespace emulator
{
    uint8_t MMU::read(uint16_t adr)
    {
        auto p = get_host_adr(adr);

        return (p == nullptr) ? 0xFF : *p;
    }

    void MMU::write(uint16_t adr, uint8_t v)
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
            case SB:
                fmt::print("{:c}", v);
                break;
            case DMA:
                // DMA TRANSFER
                // start transfer in 4 cycles
                break;
            case BANK:
                _boot_rom_enabled = (v == 0);
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

    void MMU::load_boot_rom(std::string path)
    {
        std::ifstream input(path, std::ios::binary);

        if (!input)
        {
            fmt::print("Missing boot rom file...");
            exit(1);
        }

        input.read(reinterpret_cast<char*>(BOOT_ROM.get()), BOOT_ROM->size());
    }

    void MMU::load_game_rom(std::string path)
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

