#ifndef COPYCAT_H
#define COPYCAT_H

#include "personagem/jogador.h"

#define COPYCAT_DELAY 3

typedef struct {
    int px;
    int py;
    int direcao;
    const sprite_frame_t *frame_animacao;
    
    int fire_clique;
    int fire_forte_clique;
    float mira_dx;
    float mira_dy;
} copycat_state_t;

typedef struct {
    copycat_state_t history[COPYCAT_DELAY];
    int history_index;
    int history_count;
    
    int px;
    int py;
    int direcao;
    const sprite_frame_t *frame_atual;
    
    int vida;
    int flash_frames;
    int morto;
} copycat_t;

void copycat_iniciar(copycat_t *cc, int px, int py);
void copycat_registrar_estado_jogador(copycat_t *cc, const jogador_t *j, int fire_clique, int fire_forte_clique, float dx, float dy);
void copycat_atualizar(copycat_t *cc);
void copycat_receber_dano(copycat_t *cc, int dano);
void copycat_desenhar(const copycat_t *cc, int camera_x, int camera_y);
retangulo_t copycat_hitbox(const copycat_t *cc);

#endif
