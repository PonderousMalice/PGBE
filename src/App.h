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
        void m_resize();

        bool m_running;
        SDL_Window* m_window;
        SDL_Renderer* m_renderer;
        SDL_Texture* m_texture;
        SDL_Rect m_rect_lcd;

        std::unique_ptr<GameBoy> gb;

        // GUI
        inline static bool m_show_perf = false;
        inline static bool m_show_memory = false;
        inline static bool m_show_vram = false;
        inline static bool m_show_main_menu_bar = false;

        static inline void PerfWindow();
        static inline void MainMenuBar();
        static inline void RenderGUI();
    };
}