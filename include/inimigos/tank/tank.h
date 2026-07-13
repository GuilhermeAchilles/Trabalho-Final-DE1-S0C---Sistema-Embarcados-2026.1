#ifndef TANK_H
#define TANK_H

#include "inimigos/tank/tank_sprites.h"
#include "inimigos/goomba/goomba.h"
#include "animacao/animacao.h"
#include "cenario/cenario.h"

typedef enum {
    TANK_IDLE,
    TANK_SHOOTING,
    TANK_DEATH
} tank_estado_t;

typedef struct {
    int px, py;
    tank_estado_t estado;
    animacao_t anim_idle;
    animacao_t anim_shooting;
    const sprite_frame_t *frame_death;
    
    int timer;
    int atirou_neste_ciclo;
} tank_t;

void tank_iniciar(tank_t *t, int px, int py);
void tank_atualizar(tank_t *t, goomba_t *goombas, int num_goombas, int jogador_x);
void tank_desenhar(const tank_t *t, int camera_x, int camera_y);

#endif
