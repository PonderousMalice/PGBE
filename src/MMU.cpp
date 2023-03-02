#include "MMU.h"
#include "Timer.h"
#include "utils.h"
#include <cstring>
#include <fmt/core.h>
#include <fstream>

namespace emulator
{
    MMU::MMU() :
        m_boot_rom(std::make_unique<std::array<u8, 256>>()),
        m_ext_ram(0x2000),
        vram(std::make_unique<std::array<u8, 0x2000>>()),
        wram(std::make_unique<std::array<u8, 0x2000>>()),
        oam(std::make_unique<std::array<u8, 0x00A0>>()),
        io_reg(std::make_unique<std::array<u8, 0x0080>>()),
        hram(std::make_unique<std::array<u8, 0x007F>>()),
        ie_reg(0xE0),
        internal_div(0),
        m_dma_bus_conflict(false),
        m_STAT((STAT_REG&)io_reg->at(STAT)),
        m_select_action(false),
        m_select_direction(false),
        m_rom_gb(),
        m_has_ram(false),
        m_has_battery_buffered_ram(false),
        m_has_real_time_clock(false),
        m_has_rumble(false),
        m_has_accelerometer(false),
        m_ext_ram_enabled(false),
        m_mbc_type(None),
        m_mode_flag(false),
        m_ram_size(0),
        m_rom_size(0),
        m_ram_bank_nb(0),
        m_rom_bank_nb(1)
    {
        m_boot_rom->fill(0x00);
        vram->fill(0);
        wram->fill(0);
        oam->fill(0);
        io_reg->fill(0);
        hram->fill(0);

        io_reg->at(P1_JOYP) = 0xFF;
        timer = nullptr;

        p_input.fill(false);
    }

    u8 MMU::read(u16 adr)
    {
        if (adr == 0xFF00) // JOYPAD
        {
            u8 res = 0xFF;

            if (m_select_direction)
            {
                set_bit(res, 4, false);
                for (int i = 0; i < 4; ++i)
                {
                    if (p_input.at(i))
                    {
                        set_bit(res, i, false);
                    }
                }
            }

            if (m_select_action)
            {
                set_bit(res, 5, false);
                for (int i = 4; i < 8; ++i)
                {
                    if (p_input.at(i))
                    {
                        set_bit(res, (i % 4), false);
                    }
                }
            }

            return res;
        }

        auto p = get_host_adr(adr);

        if (m_mbc_type == MBC2)
        {
            return (0xF0 | (*p & 0xF));
        }

        return (p == nullptr) ? 0xFF : *p;
    }

    void MMU::write(u16 adr, u8 v)
    {
        if (adr == 0xFF00) // JOYPAD 
        { 
            // 0 means selected
            m_select_action = (v & 0x20) == 0;
            m_select_direction = (v & 0x10) == 0;
            return;
        }

        switch (m_mbc_type)
        {
        case MBC1:
            if (0x0000 <= adr && adr <= 0x1FFF) // Enable RAM
            {
                m_ext_ram_enabled = ((v & 0x0F) == 0x0A);
            }
            else if (0x2000 <= adr && adr <= 0x3FFF) // ROM Bank
            {
                if (v == 0)
                {
                    m_rom_bank_nb = 1;
                    return;
                }

                u8 mask = 0;
                switch (m_rom_size)
                {
                case 128:
                case 64:
                case 32:
                    mask = 0b0001'1111;
                    break;
                case 16:
                    mask = 0b0000'1111;
                    break;
                case 8:
                    mask = 0b0000'0111;
                    break;
                case 4:
                    mask = 0b0000'0011;
                    break;
                case 2:
                    mask = 0b0000'0001;
                    break;
                }

                m_rom_bank_nb = v & mask;                
            }
            else if (0x4000 <= adr && adr <= 0x5FFF) // RAM Bank
            {
                m_ram_bank_nb = v & 0b11;
            }
            else if (0x6000 <= adr && adr <= 0x7FFF) // Mode Select
            {
                m_mode_flag = ((v & 0b1) == 1);
            }
            break;
        case MBC2:
            if (0x0000 <= adr && adr <= 0x3FFF)
            {
                bool ch_ram = ((adr & 0x100) == 0);
                if (ch_ram)
                {
                    if ((v & 0xF) == 0xA)
                    {
                        m_ext_ram_enabled = true;
                    }
                    else
                    {
                        m_ext_ram_enabled = false;
                    }
                }
                else
                {
                    m_rom_bank_nb = (v == 0) ? 1 : v;
                }
            }
        break;
        case MBC3:
        // TO DO : impl rtc stuff
            if (0x0000 <= adr && adr <= 0x1FFF)
            {
                m_ext_ram_enabled = ((v & 0x0F) == 0x0A);
            }
            else if (0x2000 <= adr && adr <= 0x3FFF)
            {
                m_rom_bank_nb = (v == 0) ? 1 : (v & 0xF);
            }
            else if (0x4000 <= adr && adr <= 0x5FFF)
            {
                if (0x00 <= v && v <= 0x03)
                {
                    m_ram_bank_nb = v;
                }
            }
        break;
        case MBC5:
            if (0x0000 <= adr && adr <= 0x1FFF)
            {
                m_ext_ram_enabled = ((v & 0x0F) == 0x0A);
            }
            else if (0x2000 <= adr && adr <= 0x2FFF)
            {
                m_rom_bank_nb &= 0xFF00;
                m_rom_bank_nb |= v;
            }
            else if (0x3000 <= adr && adr <= 0x3FFF)
            {
                m_rom_bank_nb &= 0x00FF;
                m_rom_bank_nb |= ((v & 0x01) << 8);
            }
            else if (0x4000 <= adr && adr <= 0x5FFF)
            {
                m_ram_bank_nb = v;
            }
        break;
        default:
            break;
        }

        if (is_locked(adr))
        {
            return;
        }

        auto p = get_host_adr(adr);
        if (p != nullptr)
        {
            if (m_mbc_type == MBC2)
            {
                *p = (v & 0xF);
            }

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
                    fmt::print("{:c}", v);
                }
                break;
            case DMA:
                timer->schedule_task(4, std::bind(&MMU::oam_dma_transfer, this, v));
                break;
            case BANK:
                if (m_boot_rom_enabled() && v == 1)
                {
                    m_boot_rom = nullptr;
                }
                break;
            case LYC:
                m_STAT.coincidence_flag = (io_reg->at(LY) == io_reg->at(LYC));
                break;
            }
        }
    }

    u8* MMU::get_host_adr(u16 gb_adr)
    {
        if (0x0000 <= gb_adr && gb_adr <= 0x3FFF)
        {
            if (gb_adr < 0x0100 && m_boot_rom_enabled())
            {
                return m_boot_rom->data() + gb_adr;
            }

            if (m_rom_gb.empty())
            {
                return nullptr;
            }

            if (m_mode_flag)
            {
                int zero_bank_nb = 0;

                if (m_rom_size == 64)
                {
                    // not workign for MBC1M 
                    zero_bank_nb = (m_ram_bank_nb & 0b1) << 5;
                }

                if (m_rom_size == 128)
                {
                    zero_bank_nb = (m_ram_bank_nb & 0b11) << 5;
                }

                return m_rom_gb.data() + (zero_bank_nb * 0x4000) + gb_adr;
            }
            else
            {
                return m_rom_gb.data() + gb_adr;
            }
            
        }
        else if (0x4000 <= gb_adr && gb_adr <= 0x7FFF)
        {
            if (m_rom_gb.empty())
            {
                return nullptr;
            }

            if (m_rom_size == 64)
            {
                m_rom_bank_nb &= ~(0b1 << 5);
                m_rom_bank_nb |= ((m_ram_bank_nb & 0b1) << 5);
            }

            if (m_rom_size == 128)
            {
                m_rom_bank_nb &= ~(0b11 << 5);
                m_rom_bank_nb |= ((m_ram_bank_nb & 0b11) << 5);
            }

            return m_rom_gb.data() + (m_rom_bank_nb * 0x4000) + (gb_adr - 0x4000);
        }
        else if (0x8000 <= gb_adr && gb_adr <= 0x9FFF)
        {
            return vram->data() + gb_adr - 0x8000;
        }
        else if (0xA000 <= gb_adr && gb_adr <= 0xBFFF)
        {
            if (m_ext_ram_enabled)
            {
                if (m_mbc_type == MBC2)
                {
                    gb_adr &= 0x1FF;

                    return m_ext_ram.data() + gb_adr;
                }

                gb_adr -= 0xA000;
                if (m_ram_size <= 8192)
                {
                    gb_adr %= m_ram_size;
                }
                else if (m_mode_flag)
                {
                    gb_adr += 0x2000 * m_ram_bank_nb;
                }

                return m_ext_ram.data() + gb_adr;
            }
            else
            {
                return nullptr;
            }
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

        input.read(reinterpret_cast<char*>(m_boot_rom.get()), m_boot_rom->size());
    }

    void MMU::load_game_rom(std::string path)
    {
        std::ifstream input(path, std::ios::binary);
        m_rom_gb = std::vector<u8>((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

        u8 mbc_type = m_rom_gb.at(0x147);
        switch (mbc_type)
        {
        case 0x00:
            m_mbc_type = None;
            break;
        case 0x01:
            m_mbc_type = MBC1;
            break;
        case 0x02:
            m_mbc_type = MBC1;
            m_has_ram = true;
            break;
        case 0x03:
            m_mbc_type = MBC1;
            m_has_battery_buffered_ram = true;
            break;
        case 0x05:
            m_mbc_type = MBC2;
            break;
        case 0x06:
            m_mbc_type = MBC2;
            m_has_battery_buffered_ram = true;
            break;
        case 0x08:
            m_mbc_type = None;
            m_has_ram = true;
            break;
        case 0x09:
            m_mbc_type = None;
            m_has_battery_buffered_ram = true;
            break;
        case 0x0B:
            m_mbc_type = MMM01;
            break;
        case 0x0C:
            m_mbc_type = MMM01;
            m_has_ram = true;
            break;
        case 0x0D:
            m_mbc_type = MMM01;
            m_has_battery_buffered_ram = true;
            break;
        case 0x0F:
            m_mbc_type = MBC3;
            m_has_real_time_clock = true;
            break;
        case 0x10:
            m_mbc_type = MBC3;
            m_has_real_time_clock = true;
            m_has_battery_buffered_ram = true;
            break;
        case 0x11:
            m_mbc_type = MBC3;
            break;
        case 0x12:
            m_mbc_type = MBC3;
            m_has_ram = true;
            break;
        case 0x13:
            m_mbc_type = MBC3;
            m_has_battery_buffered_ram = true;
            break;
        case 0x19:
            m_mbc_type = MBC5;
            break;
        case 0x1A:
            m_mbc_type = MBC5;
            m_has_ram = true;
            break;
        case 0x1B:
            m_mbc_type = MBC5;
            m_has_battery_buffered_ram = true;
            break;
        case 0x1C:
            m_mbc_type = MBC5;
            m_has_rumble = true;
            break;
        case 0x1D:
            m_mbc_type = MBC5;
            m_has_rumble = true;
            m_has_ram = true;
            break;
        case 0x1E:
            m_mbc_type = MBC5;
            m_has_rumble = true;
            m_has_battery_buffered_ram = true;
            break;
        case 0x20:
            m_mbc_type = MBC6;
            break;
        case 0x22:
            m_mbc_type = MBC7;
            m_has_rumble = true;
            m_has_battery_buffered_ram = true;
            m_has_accelerometer = true;
            break;
        case 0xFC:
            m_mbc_type = POCKET_CAMERA;
            break;
        case 0xFD:
            m_mbc_type = BANDAI_TAMA5;
            break;
        case 0xFE:
            m_mbc_type = HUC3;
            break;
        case 0xFF:
            m_mbc_type = HUC1;
            break;
        }

        u8 rom_size = m_rom_gb.at(0x148);
        switch (rom_size)
        {
        case 0x00:
            m_rom_size = 0;
            break;
        case 0x01:
            m_rom_size = 4;
            break;
        case 0x02:
            m_rom_size = 8;
            break;
        case 0x03:
            m_rom_size = 16;
            break;
        case 0x04:
            m_rom_size = 32;
            break;
        case 0x05:
            m_rom_size = 64;
            break;
        case 0x06:
            m_rom_size = 128;
            break;
        case 0x07:
            m_rom_size = 256;
            break;
        case 0x08:
            m_rom_size = 512;
            break;
        case 0x52:
            m_rom_size = 72;
            break;
        case 0x53:
            m_rom_size = 80;
            break;
        case 0x54:
            m_rom_size = 96;
            break;
        }

        u8 ram_size = m_rom_gb.at(0x149);
        switch (ram_size)
        {
        case 0x00:
            m_ram_size = 0;
            break;
        case 0x01:
            m_ram_size = 2048; // 2 KB
            break;
        case 0x02:
            m_ram_size = 8192; // 8 KB
            break;
        case 0x03:
            m_ram_size = 32'768; // 32 KB
            break;
        case 0x04:
            m_ram_size = 131'072; // 128 KB
            break;
        case 0x05:
            m_ram_size = 65'536; // 64 KB
            break;
        }

        m_ext_ram.resize(m_ram_size);
    }

    // TO DO : impl DMA Bus Conflicts
    void MMU::oam_dma_transfer(u8 src)
    {
        std::memcpy(oam.get(), get_host_adr(src << 8), 0xA0);
    }

    bool MMU::is_locked(u16 gb_adr)
    {
        //  HRAM is not affected by DMA transfers
        return
            (gb_adr < 0x8000) // ROM 
            || (0x8000 <= gb_adr && gb_adr <= 0x9FFF && (m_STAT.ppu_mode > 2)) // VRAM
            || (0xFE00 <= gb_adr && gb_adr <= 0xFE9F && (m_STAT.ppu_mode > 1)); // OAM
    }

    bool MMU::m_boot_rom_enabled()
    {
        return (m_boot_rom != nullptr);
    }
}