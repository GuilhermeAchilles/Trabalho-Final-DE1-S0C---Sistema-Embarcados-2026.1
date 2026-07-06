#ifndef CENARIO_H
#define CENARIO_H

#include <stdint.h>
#include "framebuffer/framebuffer.h"

/* Motor de cenario com rolagem (camera) e mapa de colisao por pixel.
   Backend-agnostico: desenha so via fb_put_pixel, nunca SDL/mmap direto.
   Os dados de cada fase (bg/fg/colisao) vivem em include/cenario/faseN/. */

typedef enum {
    CENARIO_VAZIO      = 0,
    CENARIO_SOLIDO     = 1, /* chao/parede: bloqueia por todos os lados */
    CENARIO_PLATAFORMA = 2, /* pisavel por cima, atravessavel por baixo */
    CENARIO_LAVA       = 3  /* mata ao encostar */
} cenario_colisao_t;

typedef struct {
    const uint16_t *bg;      /* camada de fundo (sempre desenhada) */
    const uint16_t *fg;      /* camada de frente; pode ser NULL. 0x0000 = transparente */
    const uint8_t  *colisao; /* mapa de colisao, 1 byte por pixel do mundo */
    int largura;             /* dimensoes do MUNDO (podem ser maiores que a tela) */
    int altura;
    int camera_x;            /* canto superior esquerdo da tela dentro do mundo */
    int camera_y;
} cenario_t;

void cenario_iniciar(cenario_t *c, const uint16_t *bg, const uint16_t *fg,
                     const uint8_t *colisao, int largura, int altura);

/* Tipo de colisao no pixel (x, y) do MUNDO. Fora dos limites conta como solido. */
int cenario_colisao(const cenario_t *c, int x, int y);

/* 1 se o pixel do mundo bloqueia movimento horizontal (so CENARIO_SOLIDO). */
int cenario_solido(const cenario_t *c, int x, int y);

/* Menor Y >= start_y (varrendo pra baixo) em que ha chao (solido ou plataforma)
   na coluna x do mundo. Retorna altura-1 se nao achar. */
int cenario_chao_y(const cenario_t *c, int x, int start_y);

/* Reposiciona a camera pra centralizar (alvo_x, alvo_y), presa aos cantos do mundo. */
void cenario_centralizar_camera(cenario_t *c, int alvo_x, int alvo_y);
void cenario_desenhar_bg(const cenario_t *c);
void cenario_desenhar_fg(const cenario_t *c);

/* Desenha fundo + frente ja com o deslocamento da camera.
   Chame antes de desenhar personagens/tiros. */
void cenario_desenhar(const cenario_t *c);

#endif /* CENARIO_H */
