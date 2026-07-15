/* Inimigo boss Tank */

#ifndef TANK_H
#define TANK_H

#include "inimigos/tank/tank_sprites.h"
#include "inimigos/goomba/goomba.h"
#include "animacao/animacao.h"
#include "cenario/cenario.h"

/* Tank - boss da fase 3. Fica parado, atira e lanca goombas */
typedef enum {
    TANK_IDLE,     /* parado esperando pra atirar */
    TANK_SHOOTING, /* animacao de tiro */
    TANK_DEATH     /* destruido */
} tank_estado_t;

typedef struct {
    int px, py;
    tank_estado_t estado;
    animacao_t anim_idle;
    animacao_t anim_shooting;
    const sprite_frame_t *frame_death;
    
    int timer;              /* contador de frames pro proximo tiro */
    int atirou_neste_ciclo; /* flag pra nao atirar 2x na mesma animacao */
} tank_t;

/* Inicializa o tank na posicao (px, py) */
void tank_iniciar(tank_t *t, int px, int py);

/* Atualiza IA do tank: alterna entre idle e tiro, lanca goombas */
void tank_atualizar(tank_t *t, goomba_t *goombas, int num_goombas, int jogador_x);

/* Desenha o tank na tela */
void tank_desenhar(const tank_t *t, int camera_x, int camera_y);

#endif
