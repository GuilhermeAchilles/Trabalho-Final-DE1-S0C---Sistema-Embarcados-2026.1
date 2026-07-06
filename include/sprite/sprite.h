#ifndef SPRITE_H
#define SPRITE_H

#include <stdint.h>

/* Cor sentinela de transparencia: pixels com esse valor nao sao desenhados */
#define SPRITE_TRANSPARENT 0xF81F

typedef struct {
    int width;
    int height;
    const uint16_t *pixels;
} sprite_frame_t;

/* espelhado = 1 desenha o frame invertido horizontalmente (personagem virado pro outro lado). */
void sprite_draw(const sprite_frame_t *frame, int x, int y, int espelhado);

/* Como sprite_draw, mas pinta toda a silhueta (pixels nao-transparentes) com uma cor
   solida, ignorando a cor original - usado pra dar feedback de hit (flash vermelho). */
void sprite_draw_colorido(const sprite_frame_t *frame, int x, int y, int espelhado, uint16_t cor);

#endif
