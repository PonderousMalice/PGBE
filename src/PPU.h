#pragma once
#include "const.h"
#include "MMU.h"
#include <array>
#include <span>
#include <vector>

namespace emulator
{
    union LCD_C
    {
        struct
        {
            uint8_t bg_win_enable : 1;
            uint8_t obj_enable : 1;
            uint8_t obj_size : 1;
            uint8_t bg_tile_map_select : 1;
            uint8_t tile_data_select : 1;
            uint8_t win_enable : 1;
            uint8_t win_tile_map_select : 1;
            uint8_t ppu_enable : 1;
        };
        uint8_t v;
    };

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

    struct color
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    struct gb_px
    {
        IO_REG_CODE pal;
        int index;
    };

    class PPU
    {
    public:
        PPU(MMU* mmu);

        void tick();
        void reset();
        color get_color(int x, int y);
        bool frame_completed();
    private:
        inline static const std::array<color, 4> m_dmg_color
        {
            color
            {
                .r = 224,
                .g = 248,
                .b = 208
            }, // "White"
            color
            {
                .r = 136,
                .g = 192,
                .b = 112
            }, // "Light Grey"
            color
            {
                .r = 52,
                .g = 104,
                .b = 86
            }, // "Dark Grey"
            color
            {
                .r = 8,
                .g = 24,
                .b = 32
            }, // "Black"
        };

        LCD_C& m_LCDC;
        STAT_REG& m_STAT;
        uint8_t& m_LY, & m_SCX, & m_SCY,
            & m_IF, & m_BGP, & m_OBP0, & m_OBP1,
            & m_WY, & m_WX, & m_LYC;

        std::span<uint8_t, 0x2000> m_vram;
        std::span<uint8_t, 0x00A0> m_oam;

        std::array<gb_px, VIEWPORT_BUFFER_SIZE> m_framebuffer;
        std::vector<sprite_attributes> m_sprite_buffer;
        
        int m_cur_cycle_in_scanline;
        int m_window_line_counter;
        int m_drawing_cycle_nb;
        bool m_frame_completed;
        bool m_stat_triggered;

        enum state
        {
            OAM_SCAN = 2, // Mode 2
            DRAWING = 3, // Mode 3
            H_BLANK = 0, // Mode 0
            V_BLANK = 1, // Mode 1
        } m_state;

        void m_check_coincidence();
        void m_check_stat();

        void m_draw_scanline();
        void m_scan_oam();
        void m_switch_mode(state new_state);

        int m_get_tile_data(int tile_id, int x, int y);

        gb_px m_fetch_win(int x_pos);
        gb_px m_fetch_bg(int x_pos);
        void m_fetch_obj();

        void write_framebuffer(int x_pos, gb_px px);
    };
}
