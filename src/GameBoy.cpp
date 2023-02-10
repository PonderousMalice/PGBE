#include "GameBoy.h"
#include "utils.h"

namespace emulator
{
    GameBoy::GameBoy() :
        GameBoy("")
    {}

    GameBoy::GameBoy(std::string_view rom_path) :
        m_rom_path(rom_path),
        m_logs(LOG_FILE_PATH),
        m_mmu(),
        m_ppu(&m_mmu),
        m_timer(&m_mmu, &m_ppu),
        m_cpu(&m_mmu, &m_timer)
    {
        m_mmu.timer = &m_timer;
    }

    void GameBoy::update()
    {
        while(!m_ppu.frame_completed())
        {
            if (DEBUG_LOG_ENABLED)
            {
                m_logs << m_cpu.dump() << std::flush;
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
        m_mmu.load_boot_rom(BOOT_ROM_PATH);
        if (!m_rom_path.empty())
        {
            m_mmu.load_game_rom(m_rom_path);
        }
    }

    void GameBoy::use_button(GB_BUTTON b, bool pressed)
    {
        if (pressed)
        {
            uint8_t& r_IF = m_mmu.io_reg->at(IF);

            set_bit(r_IF, 4);
        }

        m_mmu.p_input.at(b) = pressed;
    }
}

