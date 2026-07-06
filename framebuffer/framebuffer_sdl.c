#include "framebuffer/framebuffer.h"
#include <SDL2/SDL.h>

/* Dev-only preview backend: renders the same pixel buffer the DE1-SoC
   would receive, scaled up in a window, so character code never touches SDL. */

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static fb_color_t pixels[FB_WIDTH * FB_HEIGHT];

void fb_init(void) {
    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow(
        "Metal Slug - Demake (preview SDL)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        FB_WIDTH * 3, FB_HEIGHT * 3,
        SDL_WINDOW_SHOWN
    );

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_RenderSetLogicalSize(renderer, FB_WIDTH, FB_HEIGHT);

    texture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGB565,
        SDL_TEXTUREACCESS_STREAMING, FB_WIDTH, FB_HEIGHT
    );
}

void fb_shutdown(void) {
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void fb_clear(fb_color_t color) {
    for (int i = 0; i < FB_WIDTH * FB_HEIGHT; i++) {
        pixels[i] = color;
    }
}

void fb_put_pixel(int x, int y, fb_color_t color) {
    if (x < 0 || x >= FB_WIDTH || y < 0 || y >= FB_HEIGHT) return;
    pixels[y * FB_WIDTH + x] = color;
}

void fb_present(void) {
    SDL_UpdateTexture(texture, NULL, pixels, FB_WIDTH * sizeof(fb_color_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    static Uint32 last_frame_time = 0;
    Uint32 current_time = SDL_GetTicks();
    Uint32 frame_time = current_time - last_frame_time;
    if (frame_time < 16) { // Limita a ~60 FPS
        SDL_Delay(16 - frame_time);
    }
    last_frame_time = SDL_GetTicks();
}

int fb_poll_quit(void) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) return 1;
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) return 1;
    }
    return 0;
}

int fb_key_down(fb_key_t key) {
    const uint8_t *keys = SDL_GetKeyboardState(NULL);

    switch (key) {
        case FB_KEY_UP:    return keys[SDL_SCANCODE_UP]    || keys[SDL_SCANCODE_W];
        case FB_KEY_DOWN:  return keys[SDL_SCANCODE_DOWN]  || keys[SDL_SCANCODE_S];
        case FB_KEY_LEFT:  return keys[SDL_SCANCODE_LEFT]  || keys[SDL_SCANCODE_A];
        case FB_KEY_RIGHT: return keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D];
        case FB_KEY_FIRE: {
            Uint32 botoes = SDL_GetMouseState(NULL, NULL);
            return (botoes & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
        }
        case FB_KEY_FIRE_FORTE: {
            Uint32 botoes = SDL_GetMouseState(NULL, NULL);
            return (botoes & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
        }
        case FB_KEY_JUMP:   return keys[SDL_SCANCODE_LSHIFT];
        case FB_KEY_ACTION: return keys[SDL_SCANCODE_K];
    }
    return 0;
}

void fb_mouse_pos(int *x, int *y) {
    int wx, wy;
    float lx, ly;
    SDL_GetMouseState(&wx, &wy);
    SDL_RenderWindowToLogical(renderer, wx, wy, &lx, &ly);
    *x = (int)lx;
    *y = (int)ly;
}
