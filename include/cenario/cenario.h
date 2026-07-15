/* Funcoes para desenho e gerenciamento do cenario e camera */

#ifndef CENARIO_H
#define CENARIO_H

#include <stdint.h>
#include "framebuffer/framebuffer.h"

/* Cenario do jogo com camera e mapa de colisao por pixel.
   Os dados de cada fase ficam em include/cenario/faseN/. */

typedef enum {
    CENARIO_VAZIO      = 0,
    CENARIO_SOLIDO     = 1, /* chao ou parede - bloqueia tudo */
    CENARIO_PLATAFORMA = 2, /* so bloqueia por cima */
    CENARIO_LAVA       = 3  /* mata o jogador */
} cenario_colisao_t;

typedef struct {
    const uint16_t *bg;      /* imagem de fundo */
    const uint16_t *fg;      /* imagem de frente (NULL se nao tiver) */
    const uint8_t  *colisao; /* mapa de colisao, 1 byte por pixel */
    int largura;             /* tamanho do mapa (pode ser maior que a tela) */
    int altura;
    int camera_x;            /* posicao da camera no mapa */
    int camera_y;
} cenario_t;

void cenario_iniciar(cenario_t *c, const uint16_t *bg, const uint16_t *fg,
                     const uint8_t *colisao, int largura, int altura);

/* Retorna o tipo de colisao no ponto (x, y) do mapa */
int cenario_colisao(const cenario_t *c, int x, int y);

/* Retorna 1 se o ponto e solido (bloqueia movimento lateral) */
int cenario_solido(const cenario_t *c, int x, int y);

/* Acha o chao mais proximo abaixo de start_y na coluna x */
int cenario_chao_y(const cenario_t *c, int x, int start_y);

/* Centraliza a camera no jogador, sem sair dos limites do mapa */
void cenario_centralizar_camera(cenario_t *c, int alvo_x, int alvo_y);
void cenario_desenhar_bg(const cenario_t *c);
void cenario_desenhar_fg(const cenario_t *c);

/* Desenha fundo + frente com o deslocamento da camera */
void cenario_desenhar(const cenario_t *c);

#endif
