#pragma once
#include "GameBoy.h"
#include <SDL.h>

namespace emulator
{
    class App
    {
    public:
        App();
        ~App();

        void run();
    private:
        void m_init();
        void m_update();
        void m_render();
        void m_event(SDL_Event* e);

        bool m_running;
        SDL_Window* m_window;
        SDL_Renderer* m_renderer;
        SDL_Texture* m_texture;

        std::unique_ptr<GameBoy> gb;
    };
}