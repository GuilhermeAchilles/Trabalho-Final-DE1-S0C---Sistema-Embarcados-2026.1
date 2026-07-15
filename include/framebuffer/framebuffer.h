/* Acesso e manipulacao do framebuffer de video */

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>

#define FB_WIDTH  320
#define FB_HEIGHT 240

typedef uint16_t fb_color_t; /* cor no formato RGB565 */

/* Converte RGB (0-255) pra RGB565 */
static inline fb_color_t fb_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (fb_color_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

typedef enum {
    FB_KEY_UP,
    FB_KEY_DOWN,
    FB_KEY_LEFT,
    FB_KEY_RIGHT,
    FB_KEY_FIRE,       /* tiro simples */
    FB_KEY_FIRE_FORTE, /* tiro forte (botao direito) */
    FB_KEY_JUMP,       /* pulo */
    FB_KEY_ACTION,     /* acao secundaria */
    FB_KEY_DEBUG_PASS  /* pular de fase (debug) */
} fb_key_t;

void fb_init(void);
void fb_shutdown(void);
void fb_clear(fb_color_t color);
void fb_put_pixel(int x, int y, fb_color_t color);
void fb_present(void);

/* Retorna 1 se o jogo deve fechar (ESC, fechar janela, etc) */
int fb_poll_quit(void);

/* Retorna 1 enquanto a tecla estiver pressionada */
int fb_key_down(fb_key_t key);

/* Pega a posicao do mouse na tela */
void fb_mouse_pos(int *x, int *y);

#endif
