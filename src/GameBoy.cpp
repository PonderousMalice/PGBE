#include "GameBoy.h"
#include "utils.h"

namespace emulator
{
    GameBoy::GameBoy() :
        GameBoy("")
    {}

    GameBoy::GameBoy(std::string_view rom_path) :
        m_rom_path(rom_path),
        m_logs(log_file_path),
        m_mmu(),
        m_ppu((LCD_C&)m_mmu.IO_REG->at(LCDC),
            (STAT_REG&)m_mmu.IO_REG->at(STAT),
            m_mmu.IO_REG->at(LY),
            m_mmu.IO_REG->at(SCX),
            m_mmu.IO_REG->at(SCY),
            m_mmu.IO_REG->at(IF),
            std::bit_cast<std::array<sprite_attributes, 40>*>(m_mmu.OAM.get()),
            m_mmu.VRAM.get()),
        m_timer(&m_mmu, &m_ppu),
        m_cpu(&m_mmu, &m_timer, m_mmu.IO_REG->at(IF), m_mmu.IE_REG)
    {
        m_mmu.timer = &m_timer;
    }

    void GameBoy::update()
    {
        while(!m_ppu.frame_completed())
        {
            if (g_debug)
            {
                m_logs << m_cpu.dump();
            }
            m_cpu.run();
        }
    }

    color GameBoy::get_color(int x, int y)
    {
        return m_ppu.get_color(x, y);
    }

    void GameBoy::reset_ppu()
    {
        m_ppu.reset();
    }

    void GameBoy::init()
    {
        m_mmu.load_boot_rom(boot_rom_path);
        if (!m_rom_path.empty())
        {
            m_mmu.load_game_rom(m_rom_path);
        }
    }

    void GameBoy::use_button(GB_BUTTON b, bool released)
    {
        uint8_t& r_joy = m_mmu.IO_REG->at(P1_JOYP);
        uint8_t& r_IF = m_mmu.IO_REG->at(IF);

        set_bit(r_IF, 4);
        
        bool isDirection = (b < 4);

        set_bit(r_joy, 4, isDirection); // direction
        set_bit(r_joy, 5, !isDirection); // action

        switch (b)
        {
        case emulator::GB_UP:
        case emulator::GB_SELECT:
            set_bit(r_joy, 2, released);
            break;
        case emulator::GB_DOWN:
        case emulator::GB_START:
            set_bit(r_joy, 3, released);
            break;
        case emulator::GB_RIGHT:
        case emulator::GB_A:
            set_bit(r_joy, 0, released);
            break;
        case emulator::GB_LEFT:
        case emulator::GB_B:
            set_bit(r_joy, 1, released);
            break;
        }
    }
}

