#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace emulator
{
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
            uint8_t unused : 1;

        };
        uint8_t v;
    };

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
        BGP = 0x47, // BG palette data
        OBP0 = 0x48, // OBJ palette 0
        OBP1 = 0x49, // OBJ palette 1
        WY = 0x4A, // Window Y postion
        WX = 0x4B, // Window X position +7
       
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

        std::unique_ptr<std::array<uint8_t, 0x2000>> vram;
        std::unique_ptr<std::array<uint8_t, 0x2000>> wram;
        std::unique_ptr<std::array<uint8_t, 0x00A0>> oam;
        std::unique_ptr<std::array<uint8_t, 0x0080>> io_reg;
        std::unique_ptr<std::array<uint8_t, 0x007F>> hram;
        uint16_t internal_div;
        uint8_t ie_reg;
        Timer* timer;

        std::array<bool, 8> p_input;
    private:
        std::unique_ptr<std::array<uint8_t, 0x0100>> m_boot_rom;
        std::vector<uint8_t> m_rom_gb;
        std::vector<uint8_t> m_ext_ram;

        bool m_dma_bus_conflict;

        bool m_select_action;
        bool m_select_direction;

        enum 
        {
            None, // plain rom
            MBC1,
            MBC2,
            MBC3,
            MBC5,
            MBC6,
            MBC7,
            MMM01,
            POCKET_CAMERA,
            BANDAI_TAMA5,
            HUC3,
            HUC1,
        }
        m_mbc_type;
        
        int m_rom_bank_00;
        int m_rom_bank_01;
        int m_ram_bank_00;

        int m_rom_size; // bank nb
        int m_ram_size;
        bool m_mode_flag;

        bool m_has_ram;
        bool m_has_battery_buffered_ram;
        bool m_has_real_time_clock;
        bool m_has_rumble;
        bool m_has_accelerometer;

        bool m_ext_ram_enabled;

        bool m_boot_rom_enabled();

        STAT_REG& m_STAT;
    };
}