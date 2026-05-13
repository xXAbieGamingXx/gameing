# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

 # User Guidelines

NEVER commit without permission

## Build

```sh
meson setup builddir   # first time only
meson compile -C builddir
```

Meson auto-runs `shdc.sh` to compile `rectangle.glsl` → `rectangle.glsl.h` before building. If the shader changes, a full recompile will pick it up automatically.

Run the game:
```sh
./builddir/gaming
```

## Tests

```sh
meson test -C builddir        # run all tests
./builddir/unittest           # run directly for verbose munit output
```

Tests live in `test/test.c` and use the munit framework (`test/munit.c`). `player.c` guards non-test code with `#ifndef TEST` — the test binary is compiled with `-DTEST`.

## Map tool

`tools/trenchbroom_map.c` is a standalone converter built as `map_tool`:

```sh
./builddir/map_tool <input.map> <output.lvl>
```

This converts TrenchBroom `.map` text files to the binary `.lvl` format the game loads.

## Architecture

### Data flow

TrenchBroom (level editor) → `.map` text file → `map_tool` → binary `.lvl` → loaded by `parse_map.c` at startup → uploaded to a GPU vertex buffer → rendered each frame.

### Key modules

| File | Role |
|------|------|
| `main.c` | sokol_app entry point: `init` / `frame` / `event` / `cleanup`. Owns the GPU pipeline, vertex buffer, and bindings. |
| `player.c` / `player.h` | Global `player` struct (x/y/z position, ry/rz rotation). `move_player` (WASD), `player_direction` (view vector), `check_triangle_collision` (Möller–Trumbore ray-triangle test). |
| `render.c` | `render_init` (projection matrix) and `draw_vertices` (builds view-projection, submits draw call). |
| `parse_map.c` | `get_triangles` reads a `.lvl` file into a heap-allocated `triangle_data` (caller must free). |
| `images.c` | Async texture loading via sokol_fetch + stb_image. `fetch_images` kicks off loads; `images_dowork` must be called every frame until `done_fetching` is true. |
| `plane.c` | Older per-plane transform/draw helper (currently unused by main). |
| `tools/trenchbroom_map.c` | Standalone `.map` → `.lvl` converter with world-space UV projection. |

### Coordinate system

Z-up. The camera uses `vec3(0, 0, 1)` as up. In `plane.c` the Y/Z rotation axes are intentionally swapped to compensate (`mat44_rotation_z` is used for `ry`, `mat44_rotation_y` for `rz`).

### `.lvl` binary format

Each entry is 50 bytes: three vertices, each `(float x, float y, float z, int16_t u, int16_t v)` = 17 bytes × 3 = 51... plus `uint16_t texid` to reach 50 bytes total (see `file_format.md` for the authoritative spec, including the planned header and lightmap format).

UV coordinates are world-space projected: derived from the triangle's normal, scaled ×10000 to fit in `int16_t`. If the source face has UV `(1,1)` it signals a special non-projected mode (see `file_format.md`).

### Graphics backend

sokol_gfx with `SOKOL_GLCORE` (OpenGL). The shader is compiled by sokol-shdc from `rectangle.glsl` into `rectangle.glsl.h`. The single pipeline renders all world geometry in one draw call using the MVP uniform block defined in that shader.
