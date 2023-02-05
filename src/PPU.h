#pragma once
#include <array>
#include <vector>
#include "const.h"
#include "MMU.h"

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

        std::array<sprite_attributes, 40>* m_oam;
        std::array<uint8_t, 0x2000>* m_vram;

        std::array<color, VIEWPORT_BUFFER_SIZE> m_framebuffer;
        std::vector<sprite_attributes> m_sprite_buffer;
        
        int m_cur_cycle_in_scanline;
        int m_window_line_counter;
        int m_drawing_cycle_nb;
        bool m_frame_completed;
        bool m_line_drawn;
        bool m_win_reached_once;
        bool m_fetch_win;
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
        void m_draw_line();
        void m_scan_oam();
        void m_switch_mode(state new_state);
        uint8_t m_get_tile_id(int x_pos);
        std::array<uint8_t, 2> m_get_tile_data(uint8_t tile_id);
        std::array<uint8_t, 2> m_get_obj_tile_data(uint8_t tile_id);

        fifo_entry m_get_pixel(std::array<uint8_t, 2> tile_data, int i, bool sprite, bool palette = false);

        color m_get_entry_color(fifo_entry entry);

        fifo_entry m_fetch_obj(const sprite_attributes& obj, int x_pos);
    };
}
