# PGBE
A budget Game Boy Emulator. PGBE stands for Prolix Game Boy Emulator.

I aim for reasonable compatibility and performance, I'm not an accuracy zealot.
PGBE is a toy project and, as such, should not be used to play seriously (If that makes any sense).

# How to use

You need a bios named DMG_ROM.bin in the executable's folder otherwise the program will just instantly exit.
You can drag and drop rom files to play games.

# How to build

The following libraries are required : sdl2, fmt and imgui.
You may use meson wrap to resolve missing dependencies.

```
meson wrap install <dependency> #(optional)
meson setup builddir
meson compile -C buildir
```