#include <algorithm>
#include "utils.h"
#include "PPU.h"

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
        m_oam(*(mmu->oam)),
        m_vram(*(mmu->vram)),
        m_window_line_counter(0),
        m_cur_cycle_in_scanline(0),
        m_frame_completed(false),
        m_stat_triggered(false),
        m_state(H_BLANK),
        m_drawing_cycle_nb(172)
    {
        m_framebuffer.fill(
            gb_px
            {
            .pal = BGP,
            .index = 0
            });
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

                m_draw_scanline();

                if (++m_LY >= NB_SCANLINES - 10)
                {
                    m_switch_mode(V_BLANK);
                }
                else
                {
                    m_switch_mode(OAM_SCAN);
                }

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

    void PPU::m_draw_scanline()
    {
        bool increase_win = false;

        for (int x_pos = 0; x_pos < VIEWPORT_WIDTH; ++x_pos)
        {
            bool should_fetch_win =
                (m_LCDC.win_enable == 1) &&
                (m_LCDC.bg_win_enable == 1) &&
                ((m_WX - 7) < 160) &&
                (m_WY < 144) &&
                (m_LY >= m_WY) &&
                ((m_WX < 8) || ((m_WX > 7) && (x_pos >= (m_WX - 7))));

            // win
            if (should_fetch_win)
            {
                increase_win = true;

                write_framebuffer(x_pos, m_fetch_win(x_pos));
            }

            // bg
            if ((m_LCDC.bg_win_enable == 1) && (!should_fetch_win))
            {
                write_framebuffer(x_pos, m_fetch_bg(x_pos));
            }
        }

        if (increase_win)
        {
            m_window_line_counter++;
        }

        if (m_LCDC.obj_enable == 1)
        {
            m_fetch_obj();
        }
    }

    void PPU::write_framebuffer(int x_pos, gb_px px)
    {
        m_framebuffer.at(x_pos + m_LY * VIEWPORT_WIDTH) = px;
    }

    void PPU::m_scan_oam()
    {
        m_sprite_buffer.clear();

        for (int i = 0; i < m_oam.size(); i += 4)
        {
            sprite_attributes& o = (sprite_attributes&)m_oam[i];
            bool isTall = m_LCDC.obj_size;

            bool obj_on_line =
                (o.x_pos > 0) &&
                ((m_LY + 16) >= o.y_pos) &&
                ((m_LY + 16) < (o.y_pos + (isTall ? 16 : 8))) &&
                (m_sprite_buffer.size() < 10);

            if (obj_on_line)
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
        const std::array<int, 4> bg_pal
        {
            (m_BGP & 0b11),
                ((m_BGP >> 2) & 0b11),
                ((m_BGP >> 4) & 0b11),
                ((m_BGP >> 6) & 0b11),
        };

        const std::array<int, 4> obj_pal0
        {
            (m_OBP0 & 0b11),
                ((m_OBP0 >> 2) & 0b11),
                ((m_OBP0 >> 4) & 0b11),
                ((m_OBP0 >> 6) & 0b11),
        };

        const std::array<int, 4> obj_pal1
        {
            (m_OBP1 & 0b11),
                ((m_OBP1 >> 2) & 0b11),
                ((m_OBP1 >> 4) & 0b11),
                ((m_OBP1 >> 6) & 0b11),
        };

        auto px = m_framebuffer.at(x + y * VIEWPORT_WIDTH);

        switch (px.pal)
        {
        case BGP:
            return m_dmg_color.at(bg_pal.at(px.index));
        case OBP0:
            return m_dmg_color.at(obj_pal0.at(px.index));
        case OBP1:
            return m_dmg_color.at(obj_pal1.at(px.index));
        default:
            return { .r = 0, .g = 0, .b = 0 };
        }
    }

    void PPU::reset()
    {
        m_switch_mode(OAM_SCAN);
        m_LY = 0;
        m_framebuffer.fill(
            gb_px
            {
            .pal = BGP,
            .index = 0
            });
        m_frame_completed = false;
        m_window_line_counter = 0;
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

    int PPU::m_get_tile_data(int tile_id, int x, int y)
    {
        auto tile_data = m_vram.subspan((m_LCDC.tile_data_select == 1) ? TILE_DATA_1 : TILE_DATA_2, SIZE_TILEDATA);

        if (m_LCDC.tile_data_select == 0) // 0x8800
        {
            if ((tile_id & 0x80) != 0)
            {
                tile_id &= 0x7F;
            }
            else
            {
                tile_id += 128;
            }
        }

        auto data = &tile_data[(tile_id * 16) + (2 * (y % 8))];
        x = 7 - (x % 8);

        auto pal_idx = ((*data >> x) & 1) | ((((*(data + 1)) >> x) << 1) & 2);

        return pal_idx;
    }

    gb_px PPU::m_fetch_win(int x_pos)
    {
        auto win_tile_map = m_vram.subspan((m_LCDC.win_tile_map_select == 1) ? TILE_MAP_2 : TILE_MAP_1, SIZE_TILEMAP);

        int x = x_pos - m_WX + 7;
        int y = m_window_line_counter;

        int tile_id = win_tile_map[((y / 8) * 32) + (x / 8)];

        return
        {
            .pal = BGP,
            .index = m_get_tile_data(tile_id, x, y),
        };
    }

    gb_px PPU::m_fetch_bg(int x_pos)
    {
        auto bg_tile_map = m_vram.subspan((m_LCDC.bg_tile_map_select == 1) ? TILE_MAP_2 : TILE_MAP_1, SIZE_TILEMAP);

        int x = x_pos + m_SCX;
        int y = m_LY + m_SCY;

        int tile_id = bg_tile_map[((x / 8) & 0x1F) + (32 * ((y / 8) & 0x1F))];

        return
        {
            .pal = BGP,
            .index = m_get_tile_data(tile_id, x, y),
        };
    }

    void PPU::m_fetch_obj()
    {
        m_scan_oam();

        std::reverse(m_sprite_buffer.begin(), m_sprite_buffer.end());
        std::sort(m_sprite_buffer.begin(), m_sprite_buffer.end(),
            [](sprite_attributes obj1, sprite_attributes obj2) { return obj1.x_pos > obj2.x_pos; });

        for (int i = 0; i < m_sprite_buffer.size(); ++i)
        {
            auto& obj = m_sprite_buffer.at(i);

            uint8_t obj_y = (m_LY - (obj.y_pos - 16));

            uint16_t offset = (obj.flags.y_flip == 0) ? 2 * (obj_y % 8) : 2 * (7 - (obj_y % 8));

            auto obj_pal = (obj.flags.palette_nb == 0) ? OBP0 : OBP1;

            auto tile_data = m_vram.subspan(TILE_DATA_1, SIZE_TILEDATA);
            auto tile_id = obj.tile_id;

            bool isTall = m_LCDC.obj_size;

            if (isTall)
            {
                if (obj_y < 8)
                {
                    if (obj.flags.y_flip)
                    {
                        tile_id |= 0b1;
                    }
                    else
                    {
                        tile_id &= 0xFE;
                    }
                }
                else
                {
                    if (obj.flags.y_flip)
                    {
                        tile_id &= 0xFE;
                    }
                    else
                    {
                        tile_id |= 0b1;
                    }
                }
            }

            auto data = &tile_data[(tile_id * 16) + offset];

            int base_x = obj.x_pos - 8;
            for (int i = 0; i < 8; ++i)
            {
                int x = (obj.flags.x_flip == 0) ? 7 - (i % 8) : (i % 8);

                auto pal_idx = ((*data >> x) & 1) | ((((*(data + 1)) >> x) << 1) & 2);

                if (pal_idx != 0)
                {
                    int xf = base_x + i;
                    if (xf >= 0 && xf < 160)
                    {
                        auto bg_color = m_framebuffer.at(xf + m_LY * VIEWPORT_WIDTH);
                        bool bg_color_is_white = (bg_color.index == 0);

                        if ((obj.flags.obj_to_bg_prio == 0) || bg_color_is_white)
                        {
                            write_framebuffer(xf, { .pal = obj_pal, .index = pal_idx, });
                        }
                    }
                }
            }
        }
    }
}