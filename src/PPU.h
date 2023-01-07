#pragma once
#include "MMU.h"
#include <array>
#include <vector>
#include "const.h"

namespace emulator
{
    class PPU
    {
    public:
        PPU(MMU* memory)
        {
            _window_line_counter = 0;
            _frame_completed = false;
            _drawing_cycle_nb = 172;
            _cur_cycle_in_scanline = 0;
            _state = H_BLANK;
            _framebuffer.fill(0);
            _line_drawn = false;
            OAM = (std::array<sprite_attributes, 40>*) memory->OAM.get();
            VRAM = memory->VRAM.get();
            IO_REG = memory->IO_REG.get();
        }

        void tick();
        void reset();
        color get_color(int x, int y);
        bool frame_completed();        
    private:
        std::array<sprite_attributes, 40>* OAM;
        std::array<uint8_t, 0x2000>* VRAM;
        std::array<uint8_t, 0x0080>* IO_REG;

        std::array<uint8_t, buffer_size> _framebuffer;
        std::vector<sprite_attributes> _sprite_buffer;
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

        int _cur_cycle_in_scanline;
        int _window_line_counter;
        int _drawing_cycle_nb;
        bool _frame_completed;
        bool _line_drawn;

        enum state
        {
            OAM_SCAN = 2, // Mode 2
            DRAWING = 3, // Mode 3
            H_BLANK = 0, // Mode 0
            V_BLANK = 1, // Mode 1
        } _state;

        void draw_line();
        void scan_oam();
        void switch_mode(state new_state);
        uint8_t get_tile_id(int x_pos);
        std::array<uint8_t, 2> get_tile_data(uint8_t tile_id);

        LCD_C& rLCDC()
        {
            return (LCD_C&)IO_REG->at(LCDC);
        }

        STAT_REG& rSTAT()
        {
            return (STAT_REG&)IO_REG->at(STAT);
        }

        uint8_t& rLY()
        {
            return (uint8_t&)IO_REG->at(LY);
        }

        uint8_t& rSCX()
        {
            return (uint8_t&)IO_REG->at(SCX);
        }

        uint8_t& rSCY()
        {
            return (uint8_t&)IO_REG->at(SCY);
        }

        bool window_enabled()
        {
            return rLCDC().flags.win_enable;
        }

        bool lcd_enabled()
        {
            return rLCDC().flags.lcd_ppu_enable;
        }
    };
}
