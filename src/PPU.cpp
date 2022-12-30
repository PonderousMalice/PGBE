#include "PPU.h"

namespace emulator
{
    void PPU::tick(int nb_cycle)
    {
        if (!lcd_enabled())
        {
            if (read(LY) != 0x90)
            {
                reset();
                write(LY, 0x90);
            }
            return;
        }

        if (!_line_drawn)
        {
            draw_line();
        }

        _cur_cycle_in_scanline += nb_cycle;

        auto line_nb = read(LY);

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
                ++line_nb;
                write(LY, line_nb);
                if (line_nb >= nb_scanlines - 10)
                {
                    switch_mode(V_BLANK);
                }
                else
                {
                    _line_drawn = false;
                    switch_mode(OAM_SCAN);
                }

                _cur_cycle_in_scanline = _cur_cycle_in_scanline - scanline_duration;
            }
            break;
        case V_BLANK:
            if (_cur_cycle_in_scanline >= scanline_duration)
            {
                ++line_nb;
                write(LY, line_nb);

                _cur_cycle_in_scanline = _cur_cycle_in_scanline - scanline_duration;
            }

            if (line_nb >= nb_scanlines)
            {
                _frame_completed = true;
            }
            break;
        }
    }

    void PPU::draw_line()
    {
        auto rLY = read(LY);

        if (rLY >= 144)
        {
            _line_drawn = true;
            return;
        }

        scan_oam();

        for (int x_pos = 0; x_pos < viewport_width;)
        {
            auto tile_id = get_tile_id(x_pos);
            auto tile_data = get_tile_data(tile_id);

            int i = x_pos == 0 ? 7 - (read(SCX) % 8) : 7;

            for (; i >= 0; --i)
            {
                uint8_t bit_low = (tile_data[0] & (1 << i)) >> i;
                uint8_t bit_high = (tile_data[1] & (1 << i)) >> i;
                bit_high <<= 1;

                fifo_entry p
                {
                    .type = 0, // BG
                    .palette = 0, // for sprites only
                    .color = (uint8_t)(bit_low + bit_high)
                };
                _framebuffer.at(x_pos++ + rLY * viewport_width) = bg_palette.at(p.color);
            }
        }

        _line_drawn = true;
    }

    void PPU::scan_oam()
    {
        // 80 T-Cycles
        for (int i = 0; i < oam_size; ++i)
        {
            sprite_attributes o = get_obj(i);
            bool isTall = get_lcdc_reg().flags.obj_size;

            if ((o.x_pos > 0) && ((read(LY) + 16) >= o.y_pos) && ((read(LY) + 16) < (o.y_pos + (isTall ? 16 : 8))) && _sprite_buffer.size() < 10)
            {
                _sprite_buffer.push_back(o);
            }
        }
    }

    void PPU::switch_mode(state new_state)
    {
        _state = new_state;
        auto stat = get_stat_reg();
        stat.flags.ppu_mode = new_state;
        _mmu->write(STAT, stat.v);
    }

    sprite_attributes PPU::get_obj(int idx)
    {
        auto adr = idx * 4;

        sprite_attributes o{};
        o.y_pos = _mmu->OAM->at(adr);
        o.x_pos = _mmu->OAM->at(adr + 1);
        o.tile_id = _mmu->OAM->at(adr + 2);
        o.f = _mmu->OAM->at(adr + 3);

        return o;
    }

    STAT_REG PPU::get_stat_reg()
    {
        STAT_REG reg{};

        reg.v = read(STAT);

        return reg;
    }

    LCD_C PPU::get_lcdc_reg()
    {
        LCD_C reg{};

        reg.v = read(LCDC);

        return reg;
    }

    bool PPU::frame_completed()
    {
        return _frame_completed;
    }

    color PPU::get_color(int x, int y)
    {
        return _framebuffer.at(x + y * viewport_width);
    }

    void PPU::reset()
    {
        switch_mode(OAM_SCAN);
        write(LY, 0);
        // _cur_cycle_in_scanline = 0;
        _window_line_counter = 0;
        _framebuffer.fill({ 0, 0, 0 });
        _frame_completed = false;
    }

    uint8_t PPU::get_tile_id(int x_pos)
    {
        uint16_t tile_map_adr = 0x9800;
        if (get_lcdc_reg().flags.bg_tile_map_area)
        {
            tile_map_adr = 0x9C00;
        }

        uint16_t offset_y, offset_x = ((x_pos + read(SCX)) / 8) & 0x1F;

        if (window_enabled())
        {
            offset_y = 32 * (_window_line_counter / 8);
        }
        else
        {
            offset_y = 32 * (((read(LY) + read(SCY)) & 0xFF) / 8);
        }

        tile_map_adr += (offset_x + offset_y) & 0x3FF;

        return read(tile_map_adr);
    }

    std::array<uint8_t, 2> PPU::get_tile_data(uint8_t tile_id)
    {
        std::array<uint8_t, 2> res{};

        uint16_t tile_data_adr, offset;

        if (window_enabled())
        {
            offset = 2 * (_window_line_counter % 8);
        }
        else
        {
            offset = 2 * ((read(LY) + read(SCY)) % 8);
        }

        if (get_lcdc_reg().flags.bg_win_tile_data_area)
        {
            tile_data_adr = 0x8000 + tile_id * 16;
        }
        else
        {
            tile_data_adr = 0x8800 + (int8_t)(tile_id * 16);
        }

        res[0] = read(tile_data_adr + offset);
        res[1] = read(tile_data_adr + offset + 1);

        return res;
    }


    bool PPU::lcd_enabled()
    {
        return get_lcdc_reg().flags.lcd_ppu_enable;
    }
}