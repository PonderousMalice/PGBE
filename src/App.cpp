#include "App.h"
#include "defines.h"
#include <chrono>
#include <fmt/core.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer.h"

using namespace std::chrono;

namespace emulator
{
    App::App() : gb(std::make_unique<GameBoy>()),
                 m_window(nullptr),
                 m_renderer(nullptr),
                 m_running(true)
    {
    }

    App::~App()
    {
        ImGui_ImplSDLRenderer_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        SDL_DestroyTexture(m_texture);
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
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
        {
            fmt::print("SDL could not initialize! SDL_Error: {}\n", SDL_GetError());
            exit(1);
        }

        SDL_WindowFlags windows_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        m_window = SDL_CreateWindow("Prolix GB", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, windows_flags);
        m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

        if (m_renderer == NULL)
        {
            SDL_Log("SDL could not create SDL_Renderer!");
            return exit(0);
        }

        SDL_RendererInfo info;
        SDL_GetRendererInfo(m_renderer, &info);
        SDL_Log("Current SDL_Renderer: %s", info.name);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGui::StyleColorsDark();

        ImGui_ImplSDL2_InitForSDLRenderer(m_window, m_renderer);
        ImGui_ImplSDLRenderer_Init(m_renderer);

        gb->init();

        m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    }

    void App::m_event(SDL_Event *e)
    {
        ImGui_ImplSDL2_ProcessEvent(e);

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
        SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
        SDL_RenderClear(m_renderer);

        ImGuiIO &io = ImGui::GetIO();

        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Hello, world! pute");
        ImGui::Text("Frametime: %.3f ms (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();

        ImGui::Render();

        SDL_RenderSetScale(m_renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        
        u8* pixels;
        int pitch;

        SDL_LockTexture(m_texture, NULL, (void**)&pixels, &pitch);
        for (int i = 0; i < pitch * VIEWPORT_HEIGHT; i += 3)
        {
            int j = (i / 3);
            int x = j % VIEWPORT_WIDTH, 
                y = (j - x) / VIEWPORT_WIDTH;

            auto color = gb->get_color(x, y);

            pixels[i] = color.r;
            pixels[i + 1] = color.g;
            pixels[i + 2] = color.b;
        }
        SDL_UnlockTexture(m_texture);

        int scale_factor = 4;

        SDL_Rect dst
        {
            .x = 0,
            .y = 0,
            .w = VIEWPORT_WIDTH * scale_factor,
            .h = VIEWPORT_WIDTH * scale_factor
        };

        int xcenter = WINDOW_WIDTH / 2, ycenter = WINDOW_HEIGHT / 2;
        
        SDL_RenderCopy(m_renderer, m_texture, NULL, &dst);

        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(m_renderer);

        gb->reset_ppu();
    }
}