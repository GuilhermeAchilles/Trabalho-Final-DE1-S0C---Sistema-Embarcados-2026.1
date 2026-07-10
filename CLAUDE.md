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

---

## Pedro's Task: New Enemies & Phases

**Team member**: Pedro Daniel. Responsible for new enemy types and boss implementations.
**Collaborator on code**: Antigravity AI (Pedro is not proficient in C).

All new raw sprite `.c` files live in `novos/` and **must be processed by `novos/convert_to_rgb565.py`** before being usable by the engine. That script converts Piskel's ARGB 32-bit format to RGB565 16-bit as used by the framebuffer.

---

### Feature 1: Fase 1.5 — Copycat Mini-Boss

**Status**: Assets partially ready. Code not yet written.

**Concept**: A small, fully enclosed room between Phase 1 and Phase 2. Background is a Mario Bros NES-style underground pipe room. The player must defeat a Copycat enemy to proceed through the exit pipe into Phase 2.

**Game design**:
- The room is a fixed, non-scrolling closed arena.
- The Copycat has **the same max HP as the hero** (read from `jogador->vida_maxima` or a fixed constant matching it).
- The Copycat uses **the hero's own sprite** (`achilles_*` frames) with a **purple tint** applied at draw time (simple per-pixel color modulation: keep blue channel, attenuate red and green).
- The Copycat **mirrors the hero's movements** (walks the same direction but mirrored on X) with a **small input delay** (ring buffer of ~30 frames of past hero positions/states).
- Tiros from the hero damage the Copycat normally. The Copycat does NOT shoot back — it only mirrors movement and serves as a melee/collision threat (or optionally fires mirrored shots in a future iteration).
- Defeating the Copycat triggers the exit pipe animation. The hero walks into the pipe (right side of the room) to transition to Phase 2.

**Assets in `novos/`**:
- `salaEspelhada.c` — Piskel export with **2 frames**: frame 0 = background render, frame 1 = collision map (black = solid wall/floor, transparent = walkable). Needs RGB565 conversion. The pipe/portal exit is already drawn in the background (green pipe on the right side, visible in `bitmapEspelhada.png`).
- `bitmapEspelhada.png` — Preview image of the room (left half = background art, right half = collision map in white/black).

**What is MISSING artistically**:
- ⚠️ No death/explosion animation for the Copycat. Need either a dedicated sprite or reuse of the hero's `achilles_morrer` frames (with purple tint). Decision pending.
- ⚠️ The collision map in `salaEspelhada.c` may need pixel edits to mark the pipe exit zone as a special tile (e.g. blue = passable-only-when-boss-dead trigger). Pedro will edit this in Piskel when ready.

**Files to create**:
- `src/fase15_logic.c` — Phase 1.5 loop (copycat AI, mirror delay buffer, collision with exit pipe).
- `include/inimigos/copycat/copycat.h` — Copycat enemy struct and functions.
- `inimigos/copycat/copycat.c` — Copycat update/draw implementation.
- Background converted output: run `convert_to_rgb565.py` adding a new scenario entry for `salaEspelhada.c` with 2 layers `["bg", "col"]`, scale `1.0`.
- `include/cenario/fase15/fase15.h` — exports the converted background arrays.
- Update `src/main.c` to add `ESTADO_FASE15` between `ESTADO_FASE1` and `ESTADO_FASE2`, and re-initialize the jogador (preserving vida/ammo) before calling `rodar_fase_15()`.

---

### Feature 2: Spider Jockey — Phase 2 Enemy

**Status**: Assets ready (needs RGB565 conversion). Code not yet written.

**Concept**: Replaces the generic soldiers in Phase 2. The Spider Jockey is a Minecraft-style skeleton riding a spider. It is faster, has a longer patrol range, and shoots arrows in a **parabolic arc** instead of straight tiros.

**Game design**:
- **Speed**: ~2× faster than soldiers (soldier walks ~1px/frame; spider jockey should walk ~2–2.5px/frame).
- **Patrol range**: Significantly wider horizontal range before turning (~200–300px instead of ~80px for soldiers).
- **Projectile**: Arrow (`spider_jockey_arrow.c`) travels in a parabola — apply constant `gravity_y` acceleration to `vel_y` each frame, `vel_x` constant. The arrow is fired at the hero's current position with an initial upward arc. Uses existing `tiro_t` pool but requires a new `tipo_tiro_t` that ignores vertical velocity (parabolic movement must be handled outside the generic `tiros_atualizar`, or via a custom update function in `fase2_logic.c`).
- **Animations**: 3 frames total in `spider_jockey_anm.c`:
  - Frame 0: idle pose 1
  - Frame 1: idle pose 2 (together frames 0–1 = walking animation)
  - Frame 2: attack/firing pose
- **HP**: Higher than soldiers — suggest 8 hits.
- **Does NOT drop from parachute** — spawns walking on the ground directly (or falls without parachute if off-platform).

**Assets in `novos/`**:
- `spider_jockey_anm.c` — 3 frames, 58×50px, ARGB32. Needs RGB565 conversion.
- `spider_jockey_arrow.c` — 1 frame arrow sprite. Needs RGB565 conversion.
- `spider_jockey.png` / `spider_jockey_anm.png` — preview images (already visible, not used by engine).

**What is MISSING artistically**:
- ⚠️ **Death/explosion animation** for the Spider Jockey. Currently nothing. Need at least 1–2 frames (can be a simple collapse/dust cloud). Pedro must create this in Piskel.
- ✅ **Arrow**: Single static sprite. The angle in `spider_jockey_arrow.c` is already aligned with the parabolic arc. Sprite is **mirrored horizontally** when the spider jockey faces left (handled at draw time, no extra frame needed).

**Arrow damage mechanic (Minecraft-style)**:
- Arrow only deals damage if the hero collides with the **tip side** (right side of the sprite = the pointy end; left side when mirrored).
- Damage is proportional to the **relative velocity** between the hero and the arrow at collision time: `damage = base * (1 + dot(hero_vel - arrow_vel) / scale_factor)`, clamped to a min/max range. If hero runs toward the arrow → more damage; if fleeing → less; if still → base damage.

**Files to create**:
- Update `novos/convert_to_rgb565.py` to add entries for `spider_jockey_anm.c` and `spider_jockey_arrow.c` (sprites, not backgrounds — need a simpler per-frame ARGB→RGB565 path, not the multi-layer scenario path).
- `include/inimigos/spider_jockey/spider_jockey.h` — sprite frame arrays and frame counts.
- `inimigos/spider_jockey/spider_jockey.c` — (or keep sprites header-only like `achilles_*.h`).
- Modify `src/fase2_logic.c` — swap out soldado spawner for spider jockey spawner; implement parabolic arrow update logic.

---

### Feature 3: Tank Boss + Goombas — Final Phase (Phase 3)

**Status**: Goomba asset ready (needs RGB565 conversion + rename). Tank sprite NOT yet created. Code not yet written.

**Concept**: The final boss is a tank that occupies a **separate Z-layer** (drawn behind the hero, does not collide with the hero directly). The tank fires Goombas in parabolic arcs onto the hero's layer. Goombas that land become walking enemies. The hero must **stomp 20 Goombas** (jump on top of their head) to win. Regular bullets do NOT damage Goombas. Sporadic enemy soldiers also fall from the sky.

**Game design**:

*Tank*:
- Rendered on a background layer (drawn before the hero). The hero passes "in front" of it visually — no physical collision.
- The tank periodically launches Goombas: initial `vel_x` toward hero, initial `vel_y` upward, then parabolic gravity until they land.
- The tank itself has no HP; it is defeated only when 20 Goombas are stomped.

*Goombas (walking phase)*:
- After landing, Goombas become ground-walking enemies (simple back-and-forth patrol or walk toward the hero).
- **Stomp detection**: if the hero is falling (`vel_y > 0`) and hero's bottom edge overlaps Goomba's top edge → stomp kill. Hero bounces up slightly.
- Bullets pass through Goombas (no damage from `tiros_colidir_com`).
- Contact damage: if hero touches a Goomba laterally → hero takes 1 hit.
- 5 frames in `Goomba_Enemy.c` (0-indexed): **frames 0–3 = walk cycle** (4-frame animation), **frame 4 = squished** (stomp death frame, freeze on last frame). ✅ Confirmed by Pedro.

*Extra soldiers*: Existing `inimigo_t` soldiers spawn periodically from the top of the screen (already supported by the parachute/falling state in `inimigo.c`).

**Assets in `novos/`**:
- `Goomba_Enemy.c` — 5 frames, 18×18px, ARGB32. **PROBLEM**: The `#define` names and array variable name contain spaces and special characters (Piskel exported them verbatim from the sprite name). **Must be manually renamed** before the converter script or the C compiler will reject it. Suggested rename: `GOOMBA_FRAME_COUNT`, `GOOMBA_FRAME_WIDTH`, `GOOMBA_FRAME_HEIGHT`, array name `goomba_data`.

**What is MISSING artistically**:
- ⚠️ **Tank sprite** — Pedro is creating it in Piskel (in progress). Export as `.c` from Piskel and drop in `novos/`. Needs at least 1 idle frame; optionally 1 "firing" frame.
- ✅ **Goomba frame layout** — confirmed: frames 0–3 = walk cycle, frame 4 = squished death.
- ⚠️ **Tank Z-layer rendering** — The tank must be drawn before `jogador_desenhar()` in the draw sequence (draw order = Z-order). No separate Z-layer system needed; just call `tank_desenhar()` before the hero draw call in `fase3_logic.c`.

**Files to create**:
- Fix and rename `Goomba_Enemy.c`, then run through RGB565 converter.
- `include/inimigos/goomba/goomba.h` — goomba struct, states (VOANDO=parabolic, ANDANDO=walking, MORTO=squished).
- `inimigos/goomba/goomba.c` — parabolic flight, landing detection, walk AI, stomp detection.
- `include/inimigos/tank/tank.h` — tank struct (position, fire timer, goombas_killed counter).
- `inimigos/tank/tank.c` — tank update (fire timer, launch goomba) and draw (render on bg layer).
- Modify `src/fase3_logic.c` — orchestrate tank + goombas + falling soldiers; win condition = 20 stomps.
