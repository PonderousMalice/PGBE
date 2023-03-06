#include "GameBoy.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer.h"
#include "imgui.h"
#include "integers.h"
#include <chrono>
#include <memory>
#include <SDL.h>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std::chrono;

constexpr auto WINDOW_WIDTH = 800;
constexpr auto WINDOW_HEIGHT = 720;

constexpr auto BOOT_ROM_PATH = "DMG_ROM.bin";

std::unique_ptr<PGBE::GameBoy> gb = nullptr;
double frametime = 0;

static void gb_init()
{
    gb = std::make_unique<PGBE::GameBoy>();
    gb->mmu.load_boot_rom(BOOT_ROM_PATH);
}

static void gb_reset()
{
    gb_init();
}

static void perf_window()
{
    ImGuiIO &io = ImGui::GetIO();

    ImGui::Begin("Perf Info");
    ImGui::Text("Total Frametime: %.3f ms (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::Text("GB Frametime: %.2f ms", frametime);
    ImGui::End();
}

static void main_menu_bar()
{
    ImGui::BeginMainMenuBar();
    if (ImGui::BeginMenu("File"))
    {
        ImGui::MenuItem("Open");
        ImGui::MenuItem("Open Recent");
        ImGui::MenuItem("About");
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Debug"))
    {
        ImGui::Checkbox("VRAM Viewer", &gb->show_vram);
        ImGui::Checkbox("Memory Viewer", &gb->show_memory);
        ImGui::Checkbox("Performance Info", &gb->show_perf);
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}

static void render_gui()
{
    if (gb->show_main_menu_bar)
    {
        main_menu_bar();
    }
    if (gb->show_perf)
    {
        perf_window();
    }
}

static void nsleep(u64 nanoseconds)
{
#ifdef _WIN32
    HANDLE timer = nullptr;
    LARGE_INTEGER sleepTime;

    sleepTime.QuadPart = -(nanoseconds / 100LL);

    timer = CreateWaitableTimer(nullptr, true, nullptr);
    if (timer == nullptr)
    {
        exit(1);
    }
    SetWaitableTimer(timer, &sleepTime, 0, nullptr, nullptr, false);
    if (WaitForSingleObject(timer, INFINITE) != WAIT_OBJECT_0)
    {
        exit(1);
    }
    CloseHandle(timer);
#endif
}

static void on_update()
{
    gb->update();
}

static void on_render(SDL_Renderer* renderer, SDL_Texture* texture, SDL_Rect* rect)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    ImGuiIO &io = ImGui::GetIO();

    ImGui_ImplSDLRenderer_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    render_gui();

    ImGui::Render();

    SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);

    u8 *pixels;
    int pitch;

    SDL_LockTexture(texture, nullptr, (void **)&pixels, &pitch);
    for (int i = 0; i < pitch * GB_VIEWPORT_HEIGHT; i += 3)
    {
        int j = (i / 3);
        int x = j % GB_VIEWPORT_WIDTH,
            y = (j - x) / GB_VIEWPORT_WIDTH;

        auto color = gb->get_color(x, y);

        pixels[i] = color.r;
        pixels[i + 1] = color.g;
        pixels[i + 2] = color.b;
    }
    SDL_UnlockTexture(texture);

    SDL_RenderCopy(renderer, texture, nullptr, rect);

    ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(renderer);

    gb->reset_ppu();
}

static void on_resize(SDL_Window* window, SDL_Rect* rect_lcd)
{
    int win_width, win_height;

    SDL_GetWindowSize(window, &win_width, &win_height);

    int scale_factor = win_height / GB_VIEWPORT_HEIGHT;

    int rect_width = GB_VIEWPORT_WIDTH * scale_factor;
    int rect_height = GB_VIEWPORT_HEIGHT * scale_factor;

    int xcenter = win_width / 2, ycenter = win_height / 2;

    rect_lcd->x = xcenter - (rect_width / 2);
    rect_lcd->y = ycenter - (rect_height / 2);
    rect_lcd->w = rect_width;
    rect_lcd->h = rect_height;
}

static void handle_keypress(const SDL_Keycode keycode, const bool pressed)
{
    constexpr std::array<PGBE::GB_MAP, PGBE::BUTTON_ENUM_SIZE> keymapping_array =
    {
        PGBE::GB_MAP{ .b = PGBE::GB_UP, .k = SDLK_UP },
        PGBE::GB_MAP{ .b = PGBE::GB_DOWN, .k = SDLK_DOWN },
        PGBE::GB_MAP{ .b = PGBE::GB_LEFT, .k = SDLK_LEFT },
        PGBE::GB_MAP{ .b = PGBE::GB_RIGHT, .k = SDLK_RIGHT },
        PGBE::GB_MAP{ .b = PGBE::GB_A, .k = SDLK_a },
        PGBE::GB_MAP{ .b = PGBE::GB_B, .k = SDLK_s },
        PGBE::GB_MAP{ .b = PGBE::GB_START, .k = SDLK_z },
        PGBE::GB_MAP{ .b = PGBE::GB_SELECT, .k = SDLK_x },
    };

    for (auto cur : keymapping_array)
    {
        if (cur.k == keycode)
        {
            gb->use_button(cur.b, pressed);
            return;
        }
    }
}

int main(int argc, char* argv[])
{
    bool running = true;

    gb_init();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        SDL_Log("SDL could not initialize! SDL_Error: {}\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *lcd_texture;
    SDL_Rect lcd_rect;

    SDL_WindowFlags windows_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    window = SDL_CreateWindow("Prolix GB", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, windows_flags);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (renderer == nullptr)
    {
        SDL_Log("SDL could not create SDL_Renderer!\n");
        return 1;
    }

    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer, &info);
    SDL_Log("Current SDL_Renderer: %s", info.name);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer_Init(renderer);

    lcd_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, GB_VIEWPORT_WIDTH, GB_VIEWPORT_HEIGHT);
    on_resize(window, &lcd_rect);
    
    SDL_Event e;
    while (running)
    {
        auto start = high_resolution_clock::now();
        while (SDL_PollEvent(&e))
        {
            ImGui_ImplSDL2_ProcessEvent(&e);

            switch (e.type)
            {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_DROPFILE:
                gb_reset();
                gb->load_rom(e.drop.file);
                SDL_free(e.drop.file);
                break;
            case SDL_WINDOWEVENT:
                on_resize(window, &lcd_rect);
                break;
            case SDL_KEYUP:
                handle_keypress(e.key.keysym.sym, false);
                break;
            case SDL_KEYDOWN:
                handle_keypress(e.key.keysym.sym, true);
                break;
            }
        }

        on_update();
        on_render(renderer, lcd_texture, &lcd_rect);
        auto end = high_resolution_clock::now();

        frametime = duration_cast<microseconds>(end - start) / 1.0ms;
        auto sleep_time = (nanoseconds(FRAME_DURATION_NS) - duration_cast<nanoseconds>(end - start)).count();

        if (sleep_time > 0)
        {
            nsleep(sleep_time);
        }
    }

    ImGui_ImplSDLRenderer_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyTexture(lcd_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}