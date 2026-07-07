# Terminal Font Strategy

This file records font decisions and the paths for using fonts beyond the
default M5GFX presets.

## Terminal Grid Constraint

A shell terminal must keep a fixed character grid. Cursor addressing, erase
operations, box drawing, alternate-screen applications, and terminfo all assume
that column `n` maps to a stable cell. A truly proportional terminal renderer
would break alignment for common programs such as `less`, `vim`, `htop`, and
box-drawing menus.

The project now supports both modes:

- Fixed-cell debug rendering: each logical terminal column occupies the same
  pixel width. Use this for parser, screen-model, and TUI compatibility work.
- Proportional visual rendering: terminal cells still exist in the model, but
  each row is redrawn by accumulating actual glyph advances. Use this when the
  user wants to inspect release-like visual appearance.

Proportional rendering is intentionally a display mode, not a new terminal
protocol model. Cursor addressing and wrapping still operate on logical columns.
When a cell changes in proportional mode, later glyphs in the row can shift, so
the renderer repaints the affected row instead of doing a single-cell pixel
update.

## Current Built-In Font Faces

`include/app_config.h` defines these font face selectors:

- `TERMINAL_FONT_FACE_ASCII8X16=0`
- `TERMINAL_FONT_FACE_DEJAVU18=1`

It also defines rendering controls:

- `TERMINAL_RENDER_MODE_FIXED_CELL=0`
- `TERMINAL_RENDER_MODE_PROPORTIONAL=1`
- `TERMINAL_FORCE_FIXED_CELL_RENDERING=1` by default

Default formal firmware uses:

- `TERMINAL_FONT_FACE_DEJAVU18`
- `TERMINAL_RENDER_MODE_PROPORTIONAL`
- `TERMINAL_FORCE_FIXED_CELL_RENDERING=1`
- `TERMINAL_CELL_WIDTH=18`
- `TERMINAL_CELL_HEIGHT=20`
- `TERMINAL_TEXT_SIZE=1`

This keeps the accepted DejaVu18 glyph appearance but places every glyph in a
fixed-width cell. The logical terminal geometry is `69x32` on the current Tab5
layout. The formal default was changed back to fixed cells after measurement
showed the proportional renderer consuming only about 64 printable bytes per
second: each byte erased the cursor, rendered the changed cell, and redrew the
cursor, and every one of those operations repainted the entire row.

After the fixed-cell formal firmware was built and flashed, the same Module LLM
single-`printf` test containing 64 printable characters completed in about
0.016 seconds at the Tab5 post-render USB observation point, or about
4000 characters per second. The proportional result was 1.000 second, or about
64 characters per second. This is roughly a 62.5x improvement without changing
the font face, font size, terminal geometry, or 921600 login-UART rate.

The proportional preview environment uses:

- PlatformIO env `tab5_terminal_font_prop_preview`
- `TERMINAL_FONT_FACE_DEJAVU18`
- `TERMINAL_RENDER_MODE_PROPORTIONAL`
- `TERMINAL_FORCE_FIXED_CELL_RENDERING=0`
- `TERMINAL_CELL_WIDTH=18`
- `TERMINAL_CELL_HEIGHT=20`
- CDC injection enabled

`fonts::DejaVu18` is a smoother M5GFX built-in proportional GFXfont. In this
environment, narrow glyphs such as `i`, `l`, and `!` use less horizontal space
than wide glyphs such as `M` and `W`.

The fixed-cell environments are:

- PlatformIO env `tab5_min_uart_terminal` for the formal firmware
- PlatformIO env `tab5_min_uart_terminal_fixed_debug` for real UART-shell tests
- PlatformIO env `tab5_terminal_font_prop_fixed_debug`
- same `DejaVu18` font and `18x20` geometry
- `TERMINAL_FORCE_FIXED_CELL_RENDERING=1`

Use this when validating terminal behavior with a proportional-looking font
without allowing glyph advances to shift later text in a row.

## External Font Options

M5GFX supports fonts beyond the compiled-in presets through two practical
routes.

### Compile-Time GFXfont

Convert a TTF/OTF font into an Adafruit-style `GFXfont` C header, add it under
the project, declare it as an `lgfx::GFXfont`, then call:

```cpp
M5.Display.setFont(&MyTerminalFont20);
```

Suggested placement:

- `include/fonts/<font_name>.h` for the declaration
- `src/fonts/<font_name>.cpp` for the bitmap/glyph arrays, if the generated
  header is split

Pros:

- No filesystem setup.
- Deterministic firmware image.
- Good for the eventual production terminal font.

Cons:

- Firmware flash grows with every glyph.
- Glyph coverage is fixed at build time.
- Full CJK coverage is too large for this route unless heavily subsetted.

### Runtime VLW/BFF Font

Local M5GFX exposes runtime font APIs on `LGFXBase`:

```cpp
bool loadFont(const uint8_t* array, IFont::font_type_t font_type = IFont::font_type_t::ft_vlw);
bool loadFont(const char *path, IFont::font_type_t font_type = IFont::font_type_t::ft_vlw);
bool loadFont(T &fs, const char *path, IFont::font_type_t font_type = IFont::font_type_t::ft_vlw);
bool loadFont(DataWrapper* data, IFont::font_type_t font_type = IFont::font_type_t::ft_vlw);
void unloadFont(void);
```

The current M5GFX implementation creates a `VLWfont` by default, and creates a
`BFFfont` when `font_type` is `ft_lvgl`.

This means a later build can load a font from LittleFS, SD, or another
`DataWrapper` source instead of compiling all glyph data into C++.

Pros:

- Fonts can be swapped without recompiling firmware.
- Better for experiments and larger glyph sets.

Cons:

- Requires a filesystem/storage deployment path.
- Needs boot-time error handling if the font file is missing or corrupt.
- Runtime font memory use must be tested on Tab5.

## Recommended Next Font Path

The DejaVu18 font face remains the formal default. Fixed-cell rendering is the
current production mode for performance and terminal-grid correctness. The
accepted proportional appearance remains available through
`tab5_terminal_font_prop_preview`; revisit it after dirty-row batching removes
the per-byte whole-row redraw cost.
