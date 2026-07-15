/* Utilidade: Logica de execucao e troca de frames das animacoes 2D */
#include "animacao/animacao.h"

/* Inicializa a estrutura de animacao com os frames e configuracoes de repeticao (loop) */
void animacao_iniciar(animacao_t *anim, const sprite_frame_t *frames, int frame_count, int frames_per_sprite, int loop) {
    anim->frames = frames;
    anim->frame_count = frame_count;
    anim->frames_per_sprite = frames_per_sprite;
    anim->loop = loop;
    animacao_reiniciar(anim);
}

/* Reinicia a animacao, voltando para o primeiro frame */
void animacao_reiniciar(animacao_t *anim) {
    anim->frame_atual = 0;
    anim->contador = 0;
}

/* Atualiza o contador de frames. Se atingir o tempo limite, passa pro proximo sprite */
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

/* Retorna um ponteiro para os pixels do frame atual sendo desenhado */
const sprite_frame_t *animacao_frame_atual(const animacao_t *anim) {
    return &anim->frames[anim->frame_atual];
}

/* Verifica se a animacao acabou (muito util para animacoes sem loop, como morrer) */
int animacao_terminou(const animacao_t *anim) {
    return !anim->loop && anim->frame_atual == anim->frame_count - 1;
}
