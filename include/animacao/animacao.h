#ifndef ANIMACAO_H
#define ANIMACAO_H

#include "sprite/sprite.h"

typedef struct {
    const sprite_frame_t *frames;
    int frame_count;
    int frames_per_sprite; /* quantos frames de jogo cada frame de sprite dura */
    int frame_atual;
    int contador;
    int loop; /* 1 = repete do inicio, 0 = trava no ultimo frame */
} animacao_t;

void animacao_iniciar(animacao_t *anim, const sprite_frame_t *frames, int frame_count, int frames_per_sprite, int loop);
void animacao_reiniciar(animacao_t *anim);
void animacao_atualizar(animacao_t *anim);
const sprite_frame_t *animacao_frame_atual(const animacao_t *anim);
int animacao_terminou(const animacao_t *anim);

#endif
