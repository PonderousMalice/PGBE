#include "GameBoy.h"

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

            double elapsed_time = duration_cast<milliseconds>(end - start).count();

            auto sleep_time = frame_duration_ms - elapsed_time;

            std::this_thread::sleep_for(sleep_time * 1ms);
        }
    }

    void GameBoy::on_init()
    {
        init_sdl();
        _mmu->load_boot_rom(boot_rom_path);
        //_mmu->load_game_rom(gb_rom_path);
        _running = true;
    }

    void GameBoy::init_sdl()
    {
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            fmt::print("SDL could not initialize! SDL_Error: {}", SDL_GetError());
            exit(1);
        }

        if (SDL_CreateWindowAndRenderer(window_width, window_height, SDL_WINDOW_RESIZABLE, &_window, &_renderer) < 0)
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
        while (!_ppu->frame_completed())
        {
            //_logs << _cpu->dump();
            _cpu->run();
            //_logs << _cpu->print_dis() << std::flush;
        }
    }

    void GameBoy::on_render()
    {
        SDL_RenderClear(_renderer);

        for (int y = 0; y < viewport_height; ++y)
        {
            for (int x = 0; x < viewport_width; ++x)
            {
                auto color = _ppu->get_color(x, y);
                SDL_SetRenderDrawColor(_renderer, color.r, color.g, color.b, 255);
                SDL_RenderDrawPoint(_renderer, x, y);
            }
        }
        _ppu->reset();
        SDL_RenderPresent(_renderer);
    }
}

