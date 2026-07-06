# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Full team-facing documentation (setup, build/debug walkthroughs, architecture, DE1-SoC porting notes, and a log of past gotchas) lives in [`docs/`](docs/README.md) — this file is the condensed version for quick AI context.

## Project

A "demake" of Metal Slug in C, built to eventually run on a DE1-SoC FPGA board (ARMv7, embedded Linux), writing frames directly into the board's VGA pixel buffer (320x240, RGB565, no window manager). During PC development, an SDL2 window stands in for that framebuffer so the game is visible while coding.

## Build & run (Windows dev machine)

Toolchain lives in MSYS2 MINGW64 (`C:\msys64\mingw64\bin`) — cmake, ninja, gcc, gdb, and SDL2 dev files are all installed there, not in a plain PowerShell/Git Bash PATH. Either open the "MSYS2 MINGW64" terminal profile (configured as the VS Code default), or prepend it to PATH manually:

```bash
export PATH="/c/msys64/mingw64/bin:/c/msys64/usr/bin:$PATH"
```

Configure + build:
```bash
cmake -S . -B build -G Ninja
cmake --build build
```

Run:
```bash
./build/metalslug.exe
```

In VS Code: `Ctrl+Shift+B` builds (task "CMake: Build"), the task "Run" builds+runs (via `Ctrl+Shift+P` → "Tasks: Run Task" → Run), and `F5` builds+runs under gdb (breakpoints work by source line — see note on `main`/`SDL_main` below).

**Before pressing `F5`**, select "Debug metalslug" in the Run and Debug dropdown. Without an explicit selection (or via the ▷ button on the editor), VS Code falls back to the C/C++ extension's generic "build active file" flow, which picks up a different, unrelated compiler (TDM-GCC) also present on this machine's PATH and fails to find `include/framebuffer.h`. Full writeup in `docs/05-problemas-resolvidos.md`.

### Switching render backend

`USE_SDL_BACKEND` (CMake option, default `ON`) selects which `framebuffer/framebuffer_*.c` implementation gets compiled in:

```bash
cmake -S . -B build -G Ninja -DUSE_SDL_BACKEND=OFF   # target build for the DE1-SoC
```

## Architecture

**Rendering and input are abstracted behind `include/framebuffer/framebuffer.h`** so game/character code never touches SDL or hardware directly — it only calls `fb_init`, `fb_clear`, `fb_put_pixel`, `fb_present`, `fb_poll_quit`, `fb_key_down(fb_key_t)`, `fb_mouse_pos(int *x, int *y)`, and builds colors with `fb_rgb(r, g, b)` (RGB565). `FB_KEY_FIRE` accepts both `Space` and the left mouse button; on the DE1-SoC backend `fb_mouse_pos` is stubbed to the screen center (no mouse on the board — aiming input there is still an open question).

Every module gets its own subfolder under `include/` for its public header(s) (e.g. `include/framebuffer/framebuffer.h`, `include/sprite/sprite.h`, `include/animacao/animacao.h`, `include/personagem/<nome>/`), separate from the module's `.c` sources, which live in a root-level folder of the same name (`framebuffer/`, `sprite/`, `animacao/`). Follow this layout for any new module.

Two interchangeable backends implement `framebuffer.h`, chosen at configure time via `USE_SDL_BACKEND`:
- `framebuffer/framebuffer_sdl.c` — dev-only preview: renders the pixel buffer into an SDL2 window (scaled 3x), used on the PC.
- `framebuffer/framebuffer_de1soc.c` — target backend: `mmap`s `/dev/mem` at the DE1-SoC's VGA pixel buffer physical address (`0x08000000`) and writes pixels directly. **Unverified against real hardware** — confirm the address/span with whoever owns the video/bridge configuration before trusting it. `fb_key_down` here is still a stub (always returns 0) — needs to be wired to the board's buttons/switches. Only compiles on Linux (uses `sys/mman.h`) — not buildable on Windows at all.

Sprite/animation stack built on top of the framebuffer, all backend-agnostic (only call `fb_put_pixel`, never SDL/mmap directly):
- `sprite/sprite.c` (`include/sprite/sprite.h`) — `sprite_frame_t { width, height, pixels }` and `sprite_draw(frame, x, y)`, which skips pixels equal to `SPRITE_TRANSPARENT` (`0xF81F`).
- `animacao/animacao.c` (`include/animacao/animacao.h`) — `animacao_t` cycles through an array of `sprite_frame_t` at a fixed number of game frames per sprite frame. `animacao_iniciar(anim, frames, count, frames_per_sprite, loop)` — `loop=1` wraps to frame 0 (idle/andar/atirar), `loop=0` freezes on the last frame (morrer); `animacao_terminou()` reports that. Call `animacao_atualizar()` once per game frame, read the current frame with `animacao_frame_atual()`.
- `include/personagem/achilles/movimentos/` — one header per animation state (`achilles_idle.h`, `achilles_andar.h`, `achilles_atirar.h`, `achilles_pulo.h`, `achilles_morrer.h`), each with raw RGB565 pixel arrays and a `sprite_frame_t achilles_<estado>_frames[]` + `ACHILLES_<ESTADO>_FRAME_COUNT`; an umbrella `achilles.h` includes all of them. All `#include "sprite/sprite.h"` rather than redefining `sprite_frame_t`/`SPRITE_TRANSPARENT`.

Generic projectile engine, also backend-agnostic, reusable by any character (not achilles-specific despite living under a shared `ataques/` name):
- `ataques/tiros/tiro.c` (`include/ataques/tiros/tiro.h`) — fixed pool of `TIRO_MAX` (16) `tiro_t` slots (no dynamic allocation). Each active tiro stores its own position, a normalized direction vector (`dx`, `dy`), and a pointer to a `tipo_tiro_t { sprite, velocidade }` — the sprite/speed data is per-character (each character defines its own `tipo_tiro_t` constant), while the pool/engine (`tiros_iniciar`/`tiros_disparar`/`tiros_atualizar`/`tiros_desenhar`) is shared. `tiros_desenhar` just calls `sprite_draw`, so a tiro's shape is a normal `sprite_frame_t` — no special-cased drawing code per shape.
- Aiming is mouse-driven: `src/main.c` computes a unit vector from the character's center to `fb_mouse_pos()` each frame and passes it to `tiros_disparar`, so shots go in any of 360°, not just left/right.

`src/main.c` picks which `animacao_t` to play via a small `estado_t` (`ESTADO_IDLE`/`ANDAR`/`ESTADO_PULAR`/`ATIRAR`/`MORRER`). Movement (arrows/WASD) always applies regardless of state; the displayed animation prioritizes morrer > atirar > pular > andar/idle, so the character can walk while shooting or jumping. `FB_KEY_JUMP` (Left Shift) → pulo (animation only, no jump physics/gravity yet), `FB_KEY_FIRE` (Space or left mouse button) → atirar + spawns a tiro (rate-limited by a `TIRO_COOLDOWN`-frame cooldown), `FB_KEY_ACTION` (K) rising edge → morrer (freezes; pressing ACTION again resets to idle — a demo hook, not real damage/death logic).

No hitbox/collision system exists yet — an earlier `hitbox.h`/`hitbox.c` was built then deliberately deleted at the user's request (they want to write collision by hand to learn it). Don't reintroduce it unprompted.

This split is why `framebuffer/`, `sprite/`, `animacao/`, `ataques/tiros/` live outside `src/` at the project root: `src/` is meant to stay just the game entry point (`main.c`) plus `src/output/`, independent of which modules/backends are linked in.

On Windows, `target_compile_definitions(metalslug PRIVATE main=SDL_main)` renames `main` to `SDL_main` (an SDL2/mingw quirk for intercepting `WinMain`) — this is guarded by `if(WIN32)` in `CMakeLists.txt` since it would break linking on Linux/ARM. When setting gdb breakpoints on Windows, break by `file:line`, not by function name `main` (that symbol resolves to the CRT wrapper, not your code).

## Conventions

- Keep this file updated when the architecture changes (new backend, new module boundaries, new build steps) — it's the source of truth for future sessions, not a one-time snapshot.
- Favor small, single-purpose functions and avoid premature abstraction (no speculative interfaces/config for cases that don't exist yet).
- Character/gameplay code must depend only on `framebuffer.h`'s API (plus `sprite.h`/`animacao.h`), never directly on SDL2 or on `/dev/mem` — that boundary is what keeps the PC and DE1-SoC builds interchangeable.
- Shared types used by more than one character (`sprite_frame_t`, `SPRITE_TRANSPARENT`) live in `sprite.h`, not duplicated inside each `sprites_<nome>.h`.
