/* Inimigo basico Goomba */

#ifndef GOOMBA_H
#define GOOMBA_H

#include "sprite/sprite.h"
#include "animacao/animacao.h"
#include "cenario/cenario.h"
#include "colisao/colisao.h"

/* Goomba - inimigo simples que anda pro lado e morre pisado */
typedef enum {
    GOOMBA_VOANDO,   /* sendo arremessado pelo tanque */
    GOOMBA_ANDANDO,  /* andando no chao */
    GOOMBA_PULANDO,
    GOOMBA_ESMAGADO, /* morto */
    GOOMBA_INATIVO
} goomba_estado_t;

typedef struct {
    float x, y;
    float vx, vy;
    int direcao; /* 1 = direita, -1 = esquerda */
    goomba_estado_t estado;
    
    animacao_t anim_andar;
    int timer_morto; /* frames restantes antes de sumir da tela */
} goomba_t;

/* Cria um goomba na posicao (x, y) com velocidade inicial (vx, vy) */
void goomba_iniciar(goomba_t *g, float x, float y, float vx, float vy);

/* Atualiza fisica e colisao do goomba com o cenario */
void goomba_atualizar(goomba_t *g, const cenario_t *cenario);

/* Desenha o goomba na tela */
void goomba_desenhar(const goomba_t *g, int camera_x, int camera_y);

/* Mata o goomba (efeito de esmagamento) */
void goomba_esmagar(goomba_t *g);

/* Retorna a hitbox do goomba */
retangulo_t goomba_hitbox(const goomba_t *g);

#endif
