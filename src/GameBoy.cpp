#include "GameBoy.h"
#include <fmt/core.h>
#include <fstream>
#include <chrono>
#include <thread>

using namespace std::chrono;

namespace emulator
{
	void GameBoy::start()
	{
		if (_running)
		{
			return;
		}

		on_init();

		SDL_Event e;

		while (_running)
		{
			auto start = high_resolution_clock::now();
			while (SDL_PollEvent(&e))
			{
				on_event(&e);
			}
			on_loop();
			on_render();
			auto end = high_resolution_clock::now();

			auto elapsed_time = end - start;

			auto sleep_time = frame_duration_ms - duration_cast<milliseconds>(elapsed_time).count();

			std::this_thread::sleep_for(sleep_time * 1ms);
		}
	}

	void GameBoy::on_init()
	{
		init_sdl();
		load_boot_rom();
		_running = true;
	}

	void GameBoy::load_boot_rom()
	{
		std::ifstream input(boot_rom_path, std::ios::binary);
		input.read(reinterpret_cast<char*>(_mmu.boot_rom.get()), _mmu.boot_rom->size());
	}

	void GameBoy::init_sdl()
	{
		if (SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			fmt::print("SDL could not initialize! SDL_Error: {}", SDL_GetError());
			exit(1);
		}
		
		if (SDL_CreateWindowAndRenderer(window_height, window_width, SDL_WINDOW_RESIZABLE, &_window, &_renderer) < 0)
		{
			fmt::print("SDL could not create window and/or renderer! SDL_Error: {}", SDL_GetError());
			exit(1);
		}

		SDL_RenderSetLogicalSize(_renderer, viewport_width, viewport_height);
	}

	void GameBoy::on_event(SDL_Event* e)
	{
		if (e->type == SDL_QUIT)
		{
			_running = false;
		}
	}

	void GameBoy::on_loop()
	{
		while (!_ppu.frame_completed())
		{
			_cpu.dump(_logs);
			// 1 T-Cycle = 4 M-Cycles
			int elapsed_m_cycles = _cpu.tick();
			_ppu.tick(elapsed_m_cycles * 4);
		}
	}

	void GameBoy::on_render()
	{
		SDL_RenderClear(_renderer);

		for (int y = 0; y < viewport_height; ++y)
		{
			for (int x = 0; x < viewport_width; ++x)
			{
				auto color = _ppu.get_color(x, y);
				SDL_SetRenderDrawColor(_renderer, color.R, color.g, color.b, 255);
				SDL_RenderDrawPoint(_renderer, x, y);
			}
		}
		_ppu.reset();
		SDL_RenderPresent(_renderer);
	}
}

