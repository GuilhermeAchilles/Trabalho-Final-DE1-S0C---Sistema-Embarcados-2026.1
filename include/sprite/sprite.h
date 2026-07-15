/* Estruturas e funcoes de desenho de sprites */

#ifndef SPRITE_H
#define SPRITE_H

#include <stdint.h>

/* Cor usada pra representar transparencia (magenta em RGB565) */
#define SPRITE_TRANSPARENT 0xF81F

typedef struct {
    int width;
    int height;
    const uint16_t *pixels;
} sprite_frame_t;

/* Desenha o frame na posicao (x, y). espelhado = 1 inverte horizontalmente */
void sprite_draw(const sprite_frame_t *frame, int x, int y, int espelhado);

/* Desenha com escala (ex: escala=2 dobra o tamanho) */
void sprite_draw_escala(const sprite_frame_t *frame, int x, int y, int espelhado, int escala);

/* Desenha a silhueta inteira com uma cor so (usado pro flash de dano) */
void sprite_draw_colorido(const sprite_frame_t *frame, int x, int y, int espelhado, uint16_t cor);

/* Igual ao colorido mas de cabeca pra baixo */
void sprite_draw_colorido_flipv(const sprite_frame_t *frame, int x, int y, int espelhado, uint16_t cor);

/* Desenha misturando a cor original com uma cor de tint. mix vai de 0.0 a 1.0 */
void sprite_draw_tint(const sprite_frame_t *frame, int x, int y, int espelhado, uint16_t tint_cor, float mix);

#endif
