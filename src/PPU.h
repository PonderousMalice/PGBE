#pragma once
#include "MMU.h"
#include <array>
#include <vector>
#include "const.h"

namespace emulator
{
    // sprites OAM data
    struct sprite_attributes
    {
        uint8_t y_pos;
        uint8_t x_pos;
        uint8_t tile_id;
        union
        {
            struct
            {
                uint8_t cgb : 4;
                uint8_t palette_nb : 1;
                uint8_t x_flip : 1;
                uint8_t y_flip : 1;
                uint8_t obj_to_bg_prio : 1;
            } flags;
            uint8_t f;
        };
    };

    // LCD Control
    union LCD_C
    {
        struct
        {
            uint8_t bg_win_enable_prio : 1;
            uint8_t obj_enable : 1;
            uint8_t obj_size : 1;
            uint8_t bg_tile_map_area : 1;
            uint8_t bg_win_tile_data_area : 1;
            uint8_t win_enable : 1;
            uint8_t win_tile_map_area : 1;
            uint8_t lcd_ppu_enable : 1;
        } flags;
        uint8_t v;
    };

    union STAT_REG
    {
        struct
        {
            uint8_t ppu_mode : 2;
            uint8_t coincidence_flag : 1;
            uint8_t mode_0_interupt : 1;
            uint8_t mode_1_interupt : 1;
            uint8_t mode_2_interupt : 1;
            uint8_t lcy_ly_interupt : 1;
            uint8_t padding : 1;

        } flags;
        uint8_t v;
    };

    struct fifo_entry
    {
        uint8_t type : 1; // 0 = BG, 1 = Sprite
        uint8_t palette : 1; // Sprites only: Selects Sprite Palette
        uint8_t color : 2; // Color in palette
    };

    struct color
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    class PPU
    {
    public:
        PPU(MMU* memory)
        {
            _mmu = memory;
            _frame_completed = false;
            switch_mode(OAM_SCAN);
            _drawing_cycle_nb = 172;
            _cur_cycle_in_scanline = 0;
        }

        void tick(int nb_cycle);
        void reset();
        color get_color(int x, int y);
        bool frame_completed();
       
    private:
        MMU* _mmu;
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

        sprite_attributes get_obj(int idx);
        STAT_REG get_stat_reg();
        LCD_C get_lcdc_reg();

        uint8_t get_tile_id(int x_pos);
        std::array<uint8_t, 2> get_tile_data(uint8_t tile_id);

        uint8_t read(uint16_t adr) const
        {
            return _mmu->read(adr);
        }

        void write(uint16_t adr, uint8_t v)
        {
            _mmu->write(adr, v);
        }

        bool window_enabled()
        {
            //return get_lcdc_reg().flags.win_enable;
            return false;
        }

        bool tile_map_area()
        {
            return get_lcdc_reg().flags.bg_tile_map_area;
        }

        bool lcd_enabled();
    };
}

