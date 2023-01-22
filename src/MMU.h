#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace emulator
{
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

    // I/O Registers - 0xFF00 -> 0xFF7F 
    enum IO_REG_CODE
    {
        // Joypad Input
        P1_JOYP = 0x00,

        // Serial Transfer
        SB = 0x01, // Serial Transfer Data
        SC = 0x02, // Serial Transfer Control

        // Timer and divider,
        DIV = 0x04, // Divider register
        TIMA = 0x05, // Timer counter
        TMA = 0x06, //Timer modulo
        TAC = 0x07, // Timer control

        // Audio
        NR52 = 0x26, // Sound on/off
        NR51 = 0x25, // Sound panning
        NR50 = 0x24, // Master volume & VIN panning
        NR10 = 0x10, // Channel 1 sweep
        NR11 = 0x11, // Channel 1 length timer & duty cycle
        NR12 = 0x12, // Channel 1 volume & envelope
        NR13 = 0x13, // Channel 1 wavelength low [write-only]
        NR14 = 0x14, // Channel 1 wavelength high & control
        NR21 = 0x16, // Channel 2 length timer & duty cycle
        NR22 = 0x17, // Channel 2 volume & envelope
        NR23 = 0x18, // Channel 2 wavelength low [write-only]
        NR24 = 0x19, // Channel 2 wavelength high & control
        NR30 = 0x1A, // Channel 3 DAC enable
        NR31 = 0x1B, // Channel 3 length timer [write-only]
        NR32 = 0x1C, // Channel 3 output level
        NR33 = 0x1D, // Channel 3 wavelength low [write-only]
        NR34 = 0x1E, // Channel 3 wavelength high & control
        NR41 = 0x20, // Channel 4 length time [write-only]
        NR42 = 0x21, // Channel 4 volume & envelope
        NR43 = 0x22, // Channel frequency & randomness
        NR44 = 0x23, // Channel 4 control

        // LCD
        LCDC = 0x40, // LCD Control
        STAT = 0x41, // LCD status
        SCY = 0x42, // Viewport Y position
        SCX = 0x43, // Viewport X postion
        LY = 0x44, // LCD Y coordinate [read-only]
        LYC = 0x45, // LY compare
        WY = 0x4A, // Window Y postion
        WX = 0x4B, // Window X position +7
        BGP = 0x47, // BG palette data

        // SPECIAL
        DMA = 0x46,
        BANK = 0x50,
        IF = 0x0F,
        IE = 0xFF
    };

    class Timer;

    class MMU
    {
    public:
        MMU();

        uint8_t read(uint16_t adr);
        void write(uint16_t adr, uint8_t v);
        uint8_t* get_host_adr(uint16_t gb_adr);
        bool is_locked(uint16_t gb_adr);

        void oam_dma_transfer(uint8_t src);

        void load_boot_rom(std::string path);
        void load_game_rom(std::string path);

        std::unique_ptr<std::array<uint8_t, 0x0100>> BOOT_ROM;
        std::unique_ptr<std::array<uint8_t, 0x4000>> ROM_BANK_00;
        std::unique_ptr<std::array<uint8_t, 0x4000>> ROM_BANK_01;
        std::unique_ptr<std::array<uint8_t, 0x2000>> VRAM;
        std::unique_ptr<std::array<uint8_t, 0x2000>> EXT_RAM;
        std::unique_ptr<std::array<uint8_t, 0x2000>> WRAM;
        std::unique_ptr<std::array<uint8_t, 0x00A0>> OAM;
        std::unique_ptr<std::array<uint8_t, 0x0080>> IO_REG;
        std::unique_ptr<std::array<uint8_t, 0x007F>> HRAM;
        uint16_t INTERNAL_DIV;
        uint8_t IE_REG;
        Timer* timer;
    private:
        bool m_dma_bus_conflict;
        bool m_boot_rom_enabled;
    };
}