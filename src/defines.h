#pragma once

namespace emulator
{
    typedef uint8_t u8;
    typedef uint16_t u16;
    typedef uint64_t u64;

    typedef int8_t i8;
    typedef int16_t i16;

    constexpr auto VIEWPORT_WIDTH = 160;
    constexpr auto VIEWPORT_HEIGHT = 144;
    constexpr auto VIEWPORT_BUFFER_SIZE = VIEWPORT_HEIGHT * VIEWPORT_WIDTH;

    constexpr auto WINDOW_WIDTH = 800;
    constexpr auto WINDOW_HEIGHT = 720;

    constexpr auto NB_SCANLINES = 154;
    constexpr auto SCANLINE_DURATION = 456; // T-Cycles
    constexpr auto FRAME_DURATION = SCANLINE_DURATION * NB_SCANLINES; // T-Cycles

    constexpr auto FREQUENCY = 4 * (1 << 20); // 4 MiHz
    constexpr auto DOT = 1.0 / FREQUENCY; // 1 T-Cycle
    constexpr auto FRAME_DURATION_NS = (u64)(DOT * FRAME_DURATION * 1'000'000'000);

    constexpr auto BOOT_ROM_PATH = "DMG_ROM.bin";
    constexpr auto LOG_FILE_PATH = "gb.log";

    constexpr auto DEBUG_LOG_ENABLED = false;

    constexpr auto VRAM_BASE = 0x8000;
    constexpr auto TILE_MAP_1 = 0x9800 - VRAM_BASE;
    constexpr auto TILE_MAP_2 = 0x9C00 - VRAM_BASE;
    constexpr auto TILE_DATA_1 = 0x8000 - VRAM_BASE;
    constexpr auto TILE_DATA_2 = 0x8800 - VRAM_BASE;

    constexpr auto SIZE_TILEMAP = 32 * 32;
    constexpr auto SIZE_TILEDATA = 384 * 16;
}
