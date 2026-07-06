#include "animacao/animacao.h"

void animacao_iniciar(animacao_t *anim, const sprite_frame_t *frames, int frame_count, int frames_per_sprite, int loop) {
    anim->frames = frames;
    anim->frame_count = frame_count;
    anim->frames_per_sprite = frames_per_sprite;
    anim->loop = loop;
    animacao_reiniciar(anim);
}

void animacao_reiniciar(animacao_t *anim) {
    anim->frame_atual = 0;
    anim->contador = 0;
}

void animacao_atualizar(animacao_t *anim) {
    if (animacao_terminou(anim)) return;

    anim->contador++;
    if (anim->contador >= anim->frames_per_sprite) {
        anim->contador = 0;
        anim->frame_atual++;
        if (anim->frame_atual >= anim->frame_count) {
            anim->frame_atual = anim->loop ? 0 : anim->frame_count - 1;
        }
    }
}

const sprite_frame_t *animacao_frame_atual(const animacao_t *anim) {
    return &anim->frames[anim->frame_atual];
}

int animacao_terminou(const animacao_t *anim) {
    return !anim->loop && anim->frame_atual == anim->frame_count - 1;
}
