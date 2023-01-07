#pragma once

#include "SM83.h"
#include "PPU.h"
#include "MMU.h"
#include "Timer.h"
#include <memory>
#include <SDL2/SDL.h>
#include <string>
#include "const.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <fmt/core.h>

namespace emulator
{
    class GameBoy
    {
    public:
        GameBoy() :
            _logs(log_file_path)
        {
            _mmu = std::make_unique<MMU>();
            _ppu = std::make_unique<PPU>(_mmu.get());
            _timer = std::make_unique<Timer>(_mmu.get(), _ppu.get());
            _cpu = std::make_unique<SM83>(_mmu.get(), _timer.get());

            _window = nullptr;
            _renderer = nullptr;
            _running = false;
        }

        ~GameBoy()
        {
            SDL_DestroyRenderer(_renderer);
            SDL_DestroyWindow(_window);
            SDL_Quit();
        }

        void start();
    private:
        std::unique_ptr<SM83> _cpu;
        std::unique_ptr<PPU> _ppu;
        std::unique_ptr<MMU> _mmu;
        std::unique_ptr<Timer> _timer;
        bool _running;
        std::ofstream _logs;

        void on_init();
        void on_event(SDL_Event* e);
        void on_loop();
        void on_render();

        void init_sdl();

        SDL_Window* _window;
        SDL_Renderer* _renderer;
    };
}