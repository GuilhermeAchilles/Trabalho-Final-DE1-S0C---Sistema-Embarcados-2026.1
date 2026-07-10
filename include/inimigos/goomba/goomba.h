#ifndef GOOMBA_H
#define GOOMBA_H

#include "sprite/sprite.h"
#include "animacao/animacao.h"
#include "cenario/cenario.h"
#include "colisao/colisao.h"

typedef enum {
    GOOMBA_VOANDO,   /* Arremessado pelo tanque */
    GOOMBA_ANDANDO,  /* Normal no chao */
    GOOMBA_PULANDO,  /* Pulando */
    GOOMBA_ESMAGADO, /* Morto */
    GOOMBA_INATIVO
} goomba_estado_t;

typedef struct {
    float x, y;
    float vx, vy;
    int direcao; /* 1 para direita, -1 para esquerda */
    goomba_estado_t estado;
    
    animacao_t anim_andar;
    int timer_morto;
} goomba_t;

void goomba_iniciar(goomba_t *g, float x, float y, float vx, float vy);
void goomba_atualizar(goomba_t *g, const cenario_t *cenario);
void goomba_desenhar(const goomba_t *g, int camera_x, int camera_y);
void goomba_esmagar(goomba_t *g);
retangulo_t goomba_hitbox(const goomba_t *g);

#endif
