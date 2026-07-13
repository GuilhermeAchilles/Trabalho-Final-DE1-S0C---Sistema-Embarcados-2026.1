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

void sprite_draw_escala(const sprite_frame_t *frame, int x, int y, int espelhado, int escala);

/* Como sprite_draw, mas pinta toda a silhueta (pixels nao-transparentes) com uma cor
   solida, ignorando a cor original - usado pra dar feedback de hit (flash vermelho). */
void sprite_draw_colorido(const sprite_frame_t *frame, int x, int y, int espelhado, uint16_t cor);
/* Como sprite_draw_colorido, mas inverte verticalmente (de cabeca para baixo). */
void sprite_draw_colorido_flipv(const sprite_frame_t *frame, int x, int y, int espelhado, uint16_t cor);

/* Desenha a sprite aplicando um tint (mescla da cor original com uma cor especifica). mix de 0.0 (original) a 1.0 (cor solida) */
void sprite_draw_tint(const sprite_frame_t *frame, int x, int y, int espelhado, uint16_t tint_cor, float mix);

#endif
