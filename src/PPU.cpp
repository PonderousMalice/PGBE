#include "PPU.h"

namespace emulator
{
    void PPU::tick()
    {
        if (!lcd_enabled())
        {
            return;
        }

        if (!_line_drawn)
        {
            draw_line();
        }

        _cur_cycle_in_scanline++;

        switch (_state)
        {
        case OAM_SCAN:
            if (_cur_cycle_in_scanline >= 80)
            {
                switch_mode(DRAWING);
            }
            break;
        case DRAWING:
            if (_cur_cycle_in_scanline >= _drawing_cycle_nb)
            {
                switch_mode(H_BLANK);
            }
            break;
        case H_BLANK:
            if (_cur_cycle_in_scanline >= scanline_duration)
            {
                _cur_cycle_in_scanline = 0;
                if (++rLY() >= nb_scanlines - 10)
                {
                    switch_mode(V_BLANK);
                }
                else
                {
                    _line_drawn = false;
                    switch_mode(OAM_SCAN);
                }
            }
            break;
        case V_BLANK:
            if (_cur_cycle_in_scanline >= scanline_duration)
            {
                _cur_cycle_in_scanline = 0;
                ++rLY();
            }

            if (rLY() >= nb_scanlines)
            {
                _frame_completed = true;
            }
            break;
        }
    }

    void PPU::draw_line()
    {
        if (rLY() >= 144)
        {
            _line_drawn = true;
            return;
        }

        scan_oam();

        for (int x_pos = 0; x_pos < viewport_width;)
        {
            auto tile_id = get_tile_id(x_pos);
            auto tile_data = get_tile_data(tile_id);

            int i = (x_pos == 0) ? 7 - (rSCX() % 8) : 7;

            for (; i >= 0; --i)
            {
                uint8_t bit_low = (tile_data.at(0) & (1 << i)) >> i;
                uint8_t bit_high = (tile_data.at(1) & (1 << i)) >> i;
                bit_high <<= 1;

                fifo_entry bg
                {
                    .type = 0, // BG
                    .palette = 0, // for sprites only
                    .color = (uint8_t)(bit_low + bit_high)
                };
                _framebuffer.at(x_pos++ + rLY() * viewport_width) = bg.color;
            }
        }

        _line_drawn = true;
    }

    void PPU::scan_oam()
    {
        // 80 T-Cycles
        for (int i = 0; i < OAM->size(); ++i)
        {
            auto o = OAM->at(i);
            bool isTall = rLCDC().flags.obj_size;

            if ((o.x_pos > 0) && ((rLY() + 16) >= o.y_pos) && ((rLY() + 16) < (o.y_pos + (isTall ? 16 : 8))) && _sprite_buffer.size() < 10)
            {
                _sprite_buffer.push_back(o);
            }
        }
    }

    void PPU::switch_mode(state new_state)
    {
        _state = new_state;
        rSTAT().flags.ppu_mode = new_state;
    }

    bool PPU::frame_completed()
    {
        return _frame_completed;
    }

    color PPU::get_color(int x, int y)
    {
        return bg_palette.at(_framebuffer.at(x + y * viewport_width));
    }

    void PPU::reset()
    {
        switch_mode(OAM_SCAN);
        rLY() = 0;
        _window_line_counter = 0;
        _framebuffer.fill(0);
        _frame_completed = false;
    }

    uint8_t PPU::get_tile_id(int x_pos)
    {
        uint16_t tile_map_adr = 0x9800;
        if (rLCDC().flags.bg_tile_map_area)
        {
            tile_map_adr = 0x9C00;
        }

        uint16_t offset_y, offset_x = ((x_pos / 8) + (rSCX() / 8)) & 0x1F;

        if (window_enabled())
        {
            offset_y = 32 * (_window_line_counter / 8);
        }
        else
        {
            offset_y = 32 * (((rLY() + rSCY()) & 0xFF) / 8);
        }

        tile_map_adr += (offset_x + offset_y) & 0x3FF;

        tile_map_adr -= 0x8000;
        return VRAM->at(tile_map_adr);
    }

    std::array<uint8_t, 2> PPU::get_tile_data(uint8_t tile_id)
    {
        uint16_t tile_data_adr, offset;

        if (window_enabled())
        {
            offset = 2 * (_window_line_counter % 8);
        }
        else
        {
            offset = 2 * ((rLY() + rSCY()) % 8);
        }

        if (rLCDC().flags.bg_win_tile_data_area)
        {
            tile_data_adr = 0x8000 + tile_id * 16;
        }
        else
        {
            tile_data_adr = 0x8800 + (int8_t)(tile_id * 16);
        }

        tile_data_adr -= 0x8000;
        
        std::array<uint8_t, 2> res
        {
            VRAM->at(tile_data_adr + offset),
            VRAM->at(tile_data_adr + offset + 1)
        };
        
        return res;
    }
}