#pragma once
#include "MMU.h"
#include <array>
#include <span>
#include <vector>

constexpr auto GB_VIEWPORT_WIDTH = 160;
constexpr auto GB_VIEWPORT_HEIGHT = 144;
constexpr auto FRAMEBUFFER_SIZE = GB_VIEWPORT_WIDTH * GB_VIEWPORT_HEIGHT;

constexpr auto VRAM_BASE = 0x8000;
constexpr auto TILE_MAP_1 = 0x9800 - VRAM_BASE;
constexpr auto TILE_MAP_2 = 0x9C00 - VRAM_BASE;
constexpr auto TILE_DATA_1 = 0x8000 - VRAM_BASE;
constexpr auto TILE_DATA_2 = 0x8800 - VRAM_BASE;

constexpr auto SIZE_TILEMAP = 32 * 32;
constexpr auto SIZE_TILEDATA = 384 * 16;

constexpr auto NB_SCANLINES = 154;
constexpr auto SCANLINE_DURATION = 456;                           // T-Cycles
constexpr auto FRAME_DURATION = SCANLINE_DURATION * NB_SCANLINES; // T-Cycles
constexpr auto FREQUENCY = 4 * (1 << 20);                         // 4 MiHz
constexpr auto DOT = (1.0 / FREQUENCY) * 1'000'000'000;           // 1 T-Cycle duration (ns)
constexpr auto FRAME_DURATION_NS = (u64)(DOT * FRAME_DURATION);

namespace PGBE
{
    union LCD_C
    {
        struct
        {
            u8 bg_win_enable : 1;
            u8 obj_enable : 1;
            u8 obj_size : 1;
            u8 bg_tile_map_select : 1;
            u8 tile_data_select : 1;
            u8 win_enable : 1;
            u8 win_tile_map_select : 1;
            u8 ppu_enable : 1;
        };
        u8 v;
    };

    struct sprite_attributes
    {
        u8 y_pos;
        u8 x_pos;
        u8 tile_id;
        union
        {
            struct
            {
                u8 cgb : 4;
                u8 palette_nb : 1;
                u8 x_flip : 1;
                u8 y_flip : 1;
                u8 obj_to_bg_prio : 1;
            } flags;
            u8 f;
        };
    };

    struct color
    {
        u8 r;
        u8 g;
        u8 b;
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
        u8& m_LY, & m_SCX, & m_SCY,
            & m_IF, & m_BGP, & m_OBP0, & m_OBP1,
            & m_WY, & m_WX, & m_LYC;

        std::span<u8, 0x2000> m_vram;
        std::span<u8, 0x00A0> m_oam;

        std::array<gb_px, FRAMEBUFFER_SIZE> m_framebuffer;
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
