#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <vector>
#include <string>

namespace emulator
{
    // I/O Registers - 0xFF00 -> 0xFF7F 
    enum IO_REG_CODE
    {
        // Joypad Input
        P1_JOYP = 0xFF00,

        // Serial Transfer
        SB = 0xFF01, // Serial Transfer Data
        SC = 0xFF02, // Serial Transfer Control

        // Timer and divider,
        DIV = 0xFF04, // Divider register
        TIMA = 0xFF05, // Timer counter
        TMA = 0xFF06, //Timer modulo
        TAC = 0xFF07, // Timer control

        // Audio
        NR52 = 0xFF26, // Sound on/off
        NR51 = 0xFF25, // Sound panning
        NR50 = 0xFF24, // Master volume & VIN panning
        NR10 = 0xFF10, // Channel 1 sweep
        NR11 = 0xFF11, // Channel 1 length timer & duty cycle
        NR12 = 0xFF12, // Channel 1 volume & envelope
        NR13 = 0xFF13, // Channel 1 wavelength low [write-only]
        NR14 = 0xFF14, // Channel 1 wavelength high & control
        NR21 = 0xFF16, // Channel 2 length timer & duty cycle
        NR22 = 0xFF17, // Channel 2 volume & envelope
        NR23 = 0xFF18, // Channel 2 wavelength low [write-only]
        NR24 = 0xFF19, // Channel 2 wavelength high & control
        NR30 = 0xFF1A, // Channel 3 DAC enable
        NR31 = 0xFF1B, // Channel 3 length timer [write-only]
        NR32 = 0xFF1C, // Channel 3 output level
        NR33 = 0xFF1D, // Channel 3 wavelength low [write-only]
        NR34 = 0xFF1E, // Channel 3 wavelength high & control
        NR41 = 0xFF20, // Channel 4 length time [write-only]
        NR42 = 0xFF21, // Channel 4 volume & envelope
        NR43 = 0xFF22, // Channel frequency & randomness
        NR44 = 0xFF23, // Channel 4 control

        // LCD
        LCDC = 0xFF40, // LCD Control
        STAT = 0xFF41, // LCD status
        SCY = 0xFF42, // Viewport Y position
        SCX = 0xFF43, // Viewport X postion
        LY = 0xFF44, // LCD Y coordinate [read-only]
        LYC = 0xFF45, // LY compare
        WY = 0xFF4A, // Window Y postion
        WX = 0xFF4B, // Window X position +7
        BGP = 0xFF47, // BG palette data

        // MISC
        TILE_MAP1 = 0x9800, //default
        TILE_MAP2 = 0x9C00,

        BANK = 0xFF50,
        CARTRIDGE_TYPE = 0x147,
        ROM_SIZE = 0x148,
        RAM_SIZE = 0x149,
    };

    class MMU
    {
    public:
        MMU()
        {
            _boot_rom = std::make_unique<std::array<uint8_t, 256>>();
            _rom_bank_00 = std::make_unique <std::array<uint8_t, 0x4000>>();
            _rom_bank_00->fill(0xFF);
            _rom_bank_01 = std::make_unique <std::array<uint8_t, 0x4000>>();
            _rom_bank_00->fill(0xFF);
            _vram = std::make_unique <std::array<uint8_t, 0x2000>>();
            _vram->fill(0);
            _external_ram = std::make_unique <std::array<uint8_t, 0x2000>>();
            _wram = std::make_unique <std::array<uint8_t, 0x2000>>();
            _wram->fill(0);
            OAM = std::make_unique <std::array<uint8_t, 0x00A0>>();
            OAM->fill(0);
            _io_reg = std::make_unique <std::array<uint8_t, 0x0080>>();
            _io_reg->fill(0);
            _hram = std::make_unique <std::array<uint8_t, 0x007F>>();
            _hram->fill(0);
            _ie_reg = 0;
            _null = 0xFF;
        }

        uint8_t read(uint16_t adr);
        void write(uint16_t adr, uint8_t v);
        uint8_t& get_host_adr(uint16_t gb_adr);

        void load_boot_rom(std::string path);
        void load_game_rom(std::string path);

        bool boot_rom_enabled()
        {
            return read(BANK) == 0;
        };

        std::unique_ptr<std::array<uint8_t, 0x00A0>> OAM;
    private:
        std::unique_ptr<std::array<uint8_t, 0x0100>> _boot_rom;
        std::unique_ptr<std::array<uint8_t, 0x4000>> _rom_bank_00;
        std::unique_ptr<std::array<uint8_t, 0x4000>> _rom_bank_01;
        std::unique_ptr<std::array<uint8_t, 0x2000>> _vram;
        std::unique_ptr<std::array<uint8_t, 0x2000>> _external_ram;
        std::unique_ptr<std::array<uint8_t, 0x2000>> _wram;
        std::unique_ptr<std::array<uint8_t, 0x0080>> _io_reg;
        std::unique_ptr<std::array<uint8_t, 0x007F>> _hram;
        uint8_t _ie_reg;
        uint8_t _null;

        std::vector<uint8_t> _rom_gb;
    };
}