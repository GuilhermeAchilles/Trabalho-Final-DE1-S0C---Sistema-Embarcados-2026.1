#include "sprite/sprite.h"
#include "framebuffer/framebuffer.h"

void sprite_draw(const sprite_frame_t *frame, int x, int y, int espelhado) {
    for (int row = 0; row < frame->height; row++) {
        for (int col = 0; col < frame->width; col++) {
            int col_origem = espelhado ? (frame->width - 1 - col) : col;
            uint16_t color = frame->pixels[row * frame->width + col_origem];
            if (color == SPRITE_TRANSPARENT) continue;
            fb_put_pixel(x + col, y + row, (fb_color_t)color);
        }
    }
}

void sprite_draw_colorido(const sprite_frame_t *frame, int x, int y, int espelhado, uint16_t cor) {
    for (int row = 0; row < frame->height; row++) {
        for (int col = 0; col < frame->width; col++) {
            int col_origem = espelhado ? (frame->width - 1 - col) : col;
            uint16_t color = frame->pixels[row * frame->width + col_origem];
            if (color == SPRITE_TRANSPARENT) continue;
            fb_put_pixel(x + col, y + row, (fb_color_t)cor);
        }
    }
}
