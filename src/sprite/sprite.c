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

void sprite_draw_escala(const sprite_frame_t *frame, int x, int y, int espelhado, int escala) {
    for (int row = 0; row < frame->height; row++) {
        for (int col = 0; col < frame->width; col++) {
            int col_origem = espelhado ? (frame->width - 1 - col) : col;
            uint16_t color = frame->pixels[row * frame->width + col_origem];
            if (color == SPRITE_TRANSPARENT) continue;
            for (int sy = 0; sy < escala; sy++) {
                for (int sx = 0; sx < escala; sx++) {
                    fb_put_pixel(x + (col * escala) + sx, y + (row * escala) + sy, (fb_color_t)color);
                }
            }
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

void sprite_draw_colorido_flipv(const sprite_frame_t *frame, int x, int y, int espelhado, uint16_t cor) {
    for (int row = 0; row < frame->height; row++) {
        for (int col = 0; col < frame->width; col++) {
            int col_origem = espelhado ? (frame->width - 1 - col) : col;
            int row_origem = frame->height - 1 - row;
            uint16_t color = frame->pixels[row_origem * frame->width + col_origem];
            if (color == SPRITE_TRANSPARENT) continue;
            fb_put_pixel(x + col, y + row, (fb_color_t)cor);
        }
    }
}

void sprite_draw_tint(const sprite_frame_t *frame, int x, int y, int espelhado, uint16_t tint_cor, float mix) {
    int tr = (tint_cor >> 11) & 0x1F;
    int tg = (tint_cor >> 5) & 0x3F;
    int tb = tint_cor & 0x1F;

    for (int row = 0; row < frame->height; row++) {
        for (int col = 0; col < frame->width; col++) {
            int col_origem = espelhado ? (frame->width - 1 - col) : col;
            uint16_t color = frame->pixels[row * frame->width + col_origem];
            if (color == SPRITE_TRANSPARENT) continue;
            
            int r = (color >> 11) & 0x1F;
            int g = (color >> 5) & 0x3F;
            int b = color & 0x1F;
            
            r = r + (int)((tr - r) * mix);
            g = g + (int)((tg - g) * mix);
            b = b + (int)((tb - b) * mix);
            
            fb_put_pixel(x + col, y + row, (fb_color_t)((r << 11) | (g << 5) | b));
        }
    }
}
