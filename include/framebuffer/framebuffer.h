#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>

#define FB_WIDTH  320
#define FB_HEIGHT 240

typedef uint16_t fb_color_t; /* RGB565 */

static inline fb_color_t fb_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (fb_color_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

typedef enum {
    FB_KEY_UP,
    FB_KEY_DOWN,
    FB_KEY_LEFT,
    FB_KEY_RIGHT,
    FB_KEY_FIRE,       /* atirar (tiro simples) */
    FB_KEY_FIRE_FORTE, /* tiro forte/carregado - botao direito do mouse */
    FB_KEY_JUMP,   /* pulo */
    FB_KEY_ACTION, /* botao de acao secundario (usado hoje so pra testar a animacao de morrer) */
    FB_KEY_DEBUG_PASS /* tecla 'ç' para pular de fase */
} fb_key_t;

void fb_init(void);
void fb_shutdown(void);
void fb_clear(fb_color_t color);
void fb_put_pixel(int x, int y, fb_color_t color);
void fb_present(void);

/* Returns 1 when the app should close (window closed / ESC / etc). */
int fb_poll_quit(void);

/* Returns 1 while the given key is held down. */
int fb_key_down(fb_key_t key);

/* Posicao do mouse em coordenadas do framebuffer (0..FB_WIDTH, 0..FB_HEIGHT). */
void fb_mouse_pos(int *x, int *y);

#endif
