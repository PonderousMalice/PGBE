#include "App.h"
#include "defines.h"
#include <chrono>
#include <fmt/core.h>

using namespace std::chrono;

namespace emulator
{
    App::App() :
        gb(std::make_unique<GameBoy>()),
        m_window(nullptr),
        m_renderer(nullptr),
        m_running(true)
    {}

    App::~App()
    {
        SDL_DestroyRenderer(m_renderer);
        SDL_DestroyWindow(m_window);
        SDL_Quit();
    }

    void App::run()
    {
        m_init();

        SDL_Event e;

        while (m_running)
        {
            auto run = high_resolution_clock::now();
            while (SDL_PollEvent(&e))
            {
                m_event(&e);
            }

            m_update();
            m_render();

            auto end = high_resolution_clock::now();
            double elapsed_time = duration_cast<milliseconds>(end - run).count();
            auto sleep_time = FRAME_DURATION_MS - elapsed_time;

            std::this_thread::sleep_for(sleep_time * 1ms);
        }
    }

    void App::m_init()
    {
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            fmt::print("SDL could not initialize! SDL_Error: {}", SDL_GetError());
            exit(1);
        }

        if (SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, &m_window, &m_renderer) < 0)
        {
            fmt::print("SDL could not create window and/or renderer! SDL_Error: {}", SDL_GetError());
            exit(1);
        }

        SDL_RenderSetLogicalSize(m_renderer, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

        gb->init();
    }

    void App::m_event(SDL_Event* e)
    {
        bool pressed = true;

        switch (e->type)
        {
        case SDL_QUIT:
            m_running = false;
            break;
        case SDL_DROPFILE:
            gb = std::make_unique<GameBoy>(e->drop.file);
            gb->init();
            SDL_free(e->drop.file);
            break;
        case SDL_KEYUP:
            pressed = false;
        case SDL_KEYDOWN:
            switch (e->key.keysym.sym)
            {
            case SDLK_UP:
                gb->use_button(GB_UP, pressed);
                break;
            case SDLK_DOWN:
                gb->use_button(GB_DOWN, pressed);
                break;
            case SDLK_LEFT:
                gb->use_button(GB_LEFT, pressed);
                break;
            case SDLK_RIGHT:
                gb->use_button(GB_RIGHT, pressed);
                break;
            case SDLK_a:
                gb->use_button(GB_A, pressed);
                break;
            case SDLK_s:
                gb->use_button(GB_B, pressed);
                break;
            case SDLK_x:
                gb->use_button(GB_SELECT, pressed);
                break;
            case SDLK_z:
                gb->use_button(GB_START, pressed);
                break;
            }
            break;
        }
    }

    void App::m_update()
    {
        gb->update();
    }

    void App::m_render()
    {
        SDL_RenderClear(m_renderer);

        for (int y = 0; y < VIEWPORT_HEIGHT; ++y)
        {
            for (int x = 0; x < VIEWPORT_WIDTH; ++x)
            {
                auto color = gb->get_color(x, y);
                SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, 255);
                SDL_RenderDrawPoint(m_renderer, x, y);
            }
        }
        gb->reset_ppu();
        SDL_RenderPresent(m_renderer);
    }
}