#pragma once
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "SM83.h"
#include "PPU.h"
#include "MMU.h"
#include <memory>
#include <SDL2/SDL.h>
#include <string>
#include "const.h"

namespace emulator
{
	class GameBoy
	{
	public:
		GameBoy() :
			_mmu(),
			_cpu(&_mmu),
			_ppu(&_mmu)
		{
			_window = nullptr;
			_renderer = nullptr;
			_running = false;
			_logs = fopen(log_file_path, "w");
		}

		~GameBoy()
		{
			fclose(_logs);
			SDL_DestroyRenderer(_renderer);
			SDL_DestroyWindow(_window);
			SDL_Quit();
		}

		void start();
	private:
		MMU _mmu;
		SM83 _cpu;
		PPU _ppu;
		bool _running;
		std::FILE* _logs;

		void on_init();
		void on_event(SDL_Event* e);
		void on_loop();
		void on_render();

		void init_sdl();
		void load_boot_rom();

		inline static const std::string boot_rom_path = "DMG_ROM.bin";
		static constexpr auto log_file_path = "gb.log";

		SDL_Window* _window;
		SDL_Renderer* _renderer;
	};
}