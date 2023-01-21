#pragma once
#include <array>
#include <vector>
#include "const.h"
#include "MMU.h"

namespace emulator
{
    class PPU
    {
    public:
        PPU(LCD_C& LCDC, STAT_REG& STAT, uint8_t& LY, uint8_t& SCX,
            uint8_t& SCY, uint8_t& rIF, std::array<sprite_attributes, 40>* OAM,
            std::array<uint8_t, 0x2000>* VRAM);

        void tick();
        void reset();
        color get_color(int x, int y);
        bool frame_completed();
    private:
        LCD_C& m_LCDC;
        STAT_REG& m_STAT;
        uint8_t& m_LY, & m_SCX, & m_SCY, &m_IF;

        std::array<sprite_attributes, 40>* m_oam;
        std::array<uint8_t, 0x2000>* m_vram;

        std::array<uint8_t, buffer_size> m_framebuffer;
        std::vector<sprite_attributes> m_sprite_buffer;
        std::array<color, 4> bg_palette
        {
            color
            {
                .r = 224,
                .g = 248,
                .b = 208
            },
            color
            {
                .r = 136,
                .g = 192,
                .b = 112
            },
            color
            {
                .r = 52,
                .g = 104,
                .b = 86
            },
            color
            {
                .r = 8,
                .g = 24,
                .b = 32
            },
        };

        int m_cur_cycle_in_scanline;
        int m_window_line_counter;
        int m_drawing_cycle_nb;
        bool m_frame_completed;
        bool m_line_drawn;

        enum state
        {
            OAM_SCAN = 2, // Mode 2
            DRAWING = 3, // Mode 3
            H_BLANK = 0, // Mode 0
            V_BLANK = 1, // Mode 1
        } m_state;

        void draw_line();
        void scan_oam();
        void switch_mode(state new_state);
        uint8_t get_tile_id(int x_pos);
        std::array<uint8_t, 2> get_tile_data(uint8_t tile_id, bool sprite = false);

        fifo_entry get_pixel(std::array<uint8_t, 2> tile_data, int i, bool sprite);

        bool window_enabled()
        {
            return m_LCDC.flags.win_enable;
        }

        bool lcd_enabled()
        {
            return m_LCDC.flags.lcd_ppu_enable;
        }
    };
}
