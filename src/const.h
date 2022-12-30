#pragma once

static constexpr auto window_width = 600;
static constexpr auto window_height = 800;
static constexpr auto framerate = 60;

static constexpr auto viewport_width = 160;
static constexpr auto viewport_height = 144;

static constexpr auto buffer_size = viewport_height * viewport_width;

static constexpr auto dot = 1.0 / (4.0 * (1 << 20)); // 4 MiHz or 1 T-Cycle
static constexpr auto nb_scanlines = 154;
static constexpr auto scanline_duration = 456; // T-Cycles
static constexpr auto frame_duration = scanline_duration * nb_scanlines; // T-Cycles

static constexpr auto frame_duration_ms = dot * frame_duration * 1000; // milliseconds

static constexpr auto oam_size = 40;
