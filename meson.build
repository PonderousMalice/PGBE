project('PGBE', 'cpp',
    version: '1.0.0',
    license: 'MIT',
    default_options: [ 
        'default_library=static',
        'cpp_std=c++20'
    ])

project_src = [
    'src/main.cpp',
    'src/imgui_impl_sdlrenderer.cpp',
    'src/imgui_impl_sdl2.cpp',
    'src/GameBoy.cpp',
    'src/MMU.cpp',
    'src/PPU.cpp',
    'src/SM83.cpp',
    'src/Timer.cpp',
]

sdl2_dep = dependency('sdl2')
dear_imgui_dep = dependency('imgui')
fmt_dep = dependency('fmt')

executable(meson.project_name(), project_src,
    win_subsystem: 'console',
    dependencies: [
        sdl2_dep,
        fmt_dep,
        dear_imgui_dep
    ])