#include "PPU.h"
#include "utils.h"

namespace emulator
{
    PPU::PPU(MMU* mmu) :
        m_LCDC((LCD_C&)mmu->io_reg->at(LCDC)),
        m_STAT((STAT_REG&)mmu->io_reg->at(STAT)),
        m_LY(mmu->io_reg->at(LY)),
        m_LYC(mmu->io_reg->at(LYC)),
        m_SCX(mmu->io_reg->at(SCX)),
        m_SCY(mmu->io_reg->at(SCY)),
        m_IF(mmu->io_reg->at(IF)),
        m_WX(mmu->io_reg->at(WX)),
        m_WY(mmu->io_reg->at(WY)),
        m_BGP(mmu->io_reg->at(BGP)),
        m_OBP0(mmu->io_reg->at(OBP0)),
        m_OBP1(mmu->io_reg->at(OBP1)),
        m_oam(std::bit_cast<std::array<sprite_attributes, 40>*>(mmu->oam.get())),
        m_vram(mmu->vram.get()),
        m_window_line_counter(0),
        m_cur_cycle_in_scanline(0),
        m_frame_completed(false),
        m_line_drawn(false),
        m_win_reached_once(false),
        m_stat_triggered(false),
        m_fetch_win(false),
        m_state(H_BLANK),
        m_drawing_cycle_nb(172)
    {
        m_framebuffer.fill({ 0, 0, 0 });
        m_sprite_buffer.reserve(10);
        m_STAT.unused = 1;
    }

    void PPU::tick()
    {
        m_check_stat();

        if (m_LCDC.ppu_enable == 0)
        {
            return;
        }

        m_cur_cycle_in_scanline++;

        switch (m_state)
        {
        case OAM_SCAN:
            if (m_cur_cycle_in_scanline >= 80)
            {
                m_switch_mode(DRAWING);
            }
            break;
        case DRAWING:
            if (m_cur_cycle_in_scanline >= m_drawing_cycle_nb)
            {
                m_switch_mode(H_BLANK);
            }
            break;
        case H_BLANK:
            if (m_cur_cycle_in_scanline >= SCANLINE_DURATION)
            {
                m_cur_cycle_in_scanline = 0;

                // delete m_line_drawn
                if (!m_line_drawn)
                {
                    m_draw_line();
                }

                if (++m_LY >= NB_SCANLINES - 10)
                {
                    m_switch_mode(V_BLANK);
                }
                else
                {
                    m_line_drawn = false;
                    m_switch_mode(OAM_SCAN);
                }

                //m_stat_triggered = false;
                m_check_coincidence();
            }
            break;
        case V_BLANK:
            if (m_cur_cycle_in_scanline >= SCANLINE_DURATION)
            {
                m_cur_cycle_in_scanline = 0;
                ++m_LY;
            }

            if (m_LY >= NB_SCANLINES)
            {
                m_frame_completed = true;
            }
            break;
        }
    }

    void PPU::m_draw_line()
    {
        if (m_LY >= 144)
        {
            m_line_drawn = true;
            return;
        }

        if (m_LY == m_WY)
        {
            m_win_reached_once = true;
        }

        m_scan_oam();

        bool increase_win_c = false;
        int nb_rendered_obj = 0;

        for (int x_pos = 0; x_pos < VIEWPORT_WIDTH; x_pos++)
        {
            if ((m_LCDC.win_enable == 1) && m_win_reached_once && (x_pos >= (m_WX - 7)))
            {
                m_fetch_win = true;
                increase_win_c = true;
            }
            else
            {
                m_fetch_win = false;
            }

            auto bg_tile_id = m_get_tile_id(x_pos);
            auto bg_tile_data = m_get_tile_data(bg_tile_id);

            auto bg_px = m_get_pixel(bg_tile_data, x_pos, false);
            m_framebuffer.at(x_pos + m_LY * VIEWPORT_WIDTH) = m_get_entry_color(bg_px);

            if (m_LCDC.obj_enable != 1)
            {
                continue;
            }

            int obj_index = -1;

            for (int z = 0; z < m_sprite_buffer.size(); ++z)
            {
                if ((m_sprite_buffer[z].x_pos - 8) == x_pos)
                {
                    obj_index = z;
                }
            }

            if (obj_index >= 0 && nb_rendered_obj < 10)
            {
                auto& obj = m_sprite_buffer[obj_index];

                for (int i = 0; i < 8; ++i)
                {
                    if ((x_pos + i) >= VIEWPORT_WIDTH)
                    {
                        break;
                    }

                    auto obj_px = m_fetch_obj(obj, (x_pos + i));

                    bg_tile_id = m_get_tile_id((x_pos + i)); // maybe useless
                    bg_px = m_get_pixel(bg_tile_data, (x_pos + i), false);

                    bool pick_bg =
                        (obj_px.color == 0) ||
                        ((obj.flags.obj_to_bg_prio == 1) && (bg_px.color != 0));

                    m_framebuffer.at((x_pos + i) + m_LY * VIEWPORT_WIDTH) = m_get_entry_color(pick_bg ? bg_px : obj_px);
                }

                nb_rendered_obj++;
                x_pos += 7;
            }
        }

        if (increase_win_c)
        {
            m_window_line_counter++;
        }

        m_line_drawn = true;
    }

    void PPU::m_scan_oam()
    {
        // 80 T-Cycles
        m_sprite_buffer.clear();

        for (int i = 0; i < m_oam->size(); ++i)
        {
            const auto& o = m_oam->at(i);
            bool isTall = m_LCDC.obj_size;

            bool cond =
                (o.x_pos > 0) &&
                ((m_LY + 16) >= o.y_pos) &&
                ((m_LY + 16) < (o.y_pos + (isTall ? 16 : 8))) &&
                (m_sprite_buffer.size() < 10);

            if (cond)
            {
                m_sprite_buffer.push_back(o);
            }
        }
    }

    void PPU::m_switch_mode(state new_state)
    {
        if (new_state == V_BLANK)
        {
            set_bit(m_IF, 0);
        }

        m_state = new_state;
        m_STAT.ppu_mode = new_state;
    }

    bool PPU::frame_completed()
    {
        return m_frame_completed;
    }

    color PPU::get_color(int x, int y)
    {
        return m_framebuffer.at(x + y * VIEWPORT_WIDTH);
    }

    void PPU::reset()
    {
        m_switch_mode(OAM_SCAN);
        m_LY = 0;
        m_window_line_counter = 0;
        m_framebuffer.fill({ 0, 0, 0 });
        m_frame_completed = false;
        m_line_drawn = false;
        m_win_reached_once = false;
    }

    uint8_t PPU::m_get_tile_id(int x_pos)
    {
        uint16_t tile_map_adr = TILE_MAP_1;
        uint16_t offset_y, offset_x;

        if (m_fetch_win)
        {
            offset_x = (x_pos / 8) & 0x1F;
            offset_y = 32 * (m_window_line_counter / 8);

            if (m_LCDC.win_tile_map_select == 1)
            {
                tile_map_adr = TILE_MAP_2;
            }
        }
        else
        {
            offset_x = ((x_pos / 8) + (m_SCX / 8)) & 0x1F;
            offset_y = 32 * (((m_LY + m_SCY) & 0xFF) / 8);

            if (m_LCDC.bg_tile_map_select == 1)
            {
                tile_map_adr = TILE_MAP_2;
            }
        }

        tile_map_adr += (offset_x + offset_y) & 0x3FF;

        return m_vram->at(tile_map_adr);
    }

    std::array<uint8_t, 2> PPU::m_get_tile_data(uint8_t tile_id)
    {
        uint16_t tile_data_adr, offset;

        if (m_fetch_win)
        {
            offset = 2 * (m_window_line_counter % 8);
        }
        else
        {
            offset = 2 * ((m_LY + m_SCY) % 8);
        }

        if (m_LCDC.tile_data_select == 1)
        {
            tile_data_adr = TILE_DATA_1 + tile_id * 16;
        }
        else
        {
            tile_data_adr = TILE_DATA_2 + (int8_t)(tile_id * 16);
        }

        std::array<uint8_t, 2> res
        {
            m_vram->at(tile_data_adr + offset),
            m_vram->at(tile_data_adr + offset + 1)
        };

        return res;
    }

    fifo_entry PPU::m_get_pixel(std::array<uint8_t, 2> tile_data, int x_pos, bool sprite, bool palette)
    {
        int i = 7 - (x_pos % 8);

        if ((x_pos < (7 - (m_SCX % 8))) && !sprite)
        {
            i = 7 - (m_SCX % 8) - x_pos;
        }

        uint8_t bit_low = (tile_data.at(0) & (1 << i)) >> i;
        uint8_t bit_high = (tile_data.at(1) & (1 << i)) >> i;
        bit_high <<= 1;

        fifo_entry entry
        {
            .type = sprite,
            .palette = palette,
            .color = (uint8_t)(bit_low + bit_high)
        };

        return entry;
    }

    color PPU::m_get_entry_color(fifo_entry entry)
    {
        int palette_color_index = (entry.color * 2), i = 0;

        if (entry.type == 0) // BG
        {
            if (m_LCDC.bg_win_enable == 1)
            {
                i = (m_BGP >> palette_color_index) & 0b11;
            }
        }
        else // Sprite
        {
            if (m_LCDC.obj_enable == 1)
            {
                if (entry.palette == 0)
                {
                    i = (m_OBP0 >> palette_color_index) & 0b11;
                }
                else
                {
                    i = (m_OBP1 >> palette_color_index) & 0b11;
                }
            }
        }

        return m_dmg_color[i];
    }

    void PPU::m_check_stat()
    {
        if (m_LCDC.ppu_enable == 0)
        {
            m_stat_triggered = false;
            return;
        }

        bool any_condition_met =
            ((m_STAT.coincidence_flag == 1) && (m_STAT.lcy_ly_interupt == 1))
            || ((m_state == OAM_SCAN) && (m_STAT.mode_2_interupt == 1))
            || ((m_state == H_BLANK) && (m_STAT.mode_0_interupt == 1))
            || ((m_state == V_BLANK) && ((m_STAT.mode_1_interupt == 1) || (m_STAT.mode_2_interupt == 1)));

        if (any_condition_met)
        {
            if (!m_stat_triggered)
            {
                set_bit(m_IF, 1);
            }

            m_stat_triggered = true;
        }
        else
        {
            m_stat_triggered = false;
        }
    }

    void PPU::m_check_coincidence()
    {
        if (m_LCDC.ppu_enable == 0)
        {
            m_STAT.coincidence_flag = 0;
            return;
        }

        if (m_LY == m_LYC)
        {
            m_STAT.coincidence_flag = 1;
        }
        else
        {
            m_STAT.coincidence_flag = 0;
        }
    }

    fifo_entry PPU::m_fetch_obj(sprite_attributes obj, int x_pos)
    {
        uint16_t offset = 2 * ((m_LY + m_SCY) % 8);

        if (obj.flags.y_flip)
        {
            offset = 2 * (7 - ((m_LY + m_SCY) % 8));
        }

        uint16_t tile_data_adr = TILE_DATA_1 + obj.tile_id * 16;

        std::array<uint8_t, 2> tile_data
        {
            m_vram->at(tile_data_adr + offset),
            m_vram->at(tile_data_adr + offset + 1)
        };

        int i = 7 - (x_pos % 8);
        if (obj.flags.x_flip)
        {
            i = (x_pos % 8);
        }

        uint8_t bit_low = (tile_data.at(0) & (1 << i)) >> i;
        uint8_t bit_high = (tile_data.at(1) & (1 << i)) >> i;
        bit_high <<= 1;

        fifo_entry entry
        {
            .type = 1,
            .palette = obj.flags.palette_nb,
            .color = (uint8_t)(bit_low + bit_high)
        };

        return entry;
    }
}