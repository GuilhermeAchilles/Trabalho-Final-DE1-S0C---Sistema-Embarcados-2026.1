/* Utilidade: Comportamento do Boss Copycat (usa buffer circular para imitar o jogador) */
#include "inimigos/copycat/copycat.h"
#include "sprite/sprite.h"
#include "framebuffer/framebuffer.h"
#include <string.h>

/* Inicializa a struct do boss Copycat (inimigo que imita as acoes do jogador com delay) */
void copycat_iniciar(copycat_t *cc, int px, int py) {
    memset(cc, 0, sizeof(copycat_t));
    cc->px = px;
    cc->py = py;
    cc->direcao = 1;
    cc->vida = 10;
    cc->morto = 0;
    cc->flash_frames = 0;
}

/* Salva o estado atual do jogador (posicao invertida, tiro, mira) num buffer circular */
void copycat_registrar_estado_jogador(copycat_t *cc, const jogador_t *j, int fire_clique, int fire_forte_clique, float dx, float dy) {
    if (cc->morto) return;
    
    copycat_state_t state;
    state.px = 320 - (int)j->px - (j->frame ? j->frame->width : 30) + 13;
    state.py = (int)j->py - 4;
    state.direcao = -j->direcao;
    state.frame_animacao = j->frame;
    state.fire_clique = fire_clique;
    state.fire_forte_clique = fire_forte_clique;
    state.mira_dx = -dx;
    state.mira_dy = dy;
    
    cc->history[cc->history_index] = state;
    cc->history_index = (cc->history_index + 1) % COPYCAT_DELAY;
    if (cc->history_count < COPYCAT_DELAY) {
        cc->history_count++;
    }
}

/* Pega o estado mais antigo salvo no buffer e aplica no Copycat, fazendo-o se mover atrasado */
void copycat_atualizar(copycat_t *cc) {
    if (cc->flash_frames > 0) cc->flash_frames--;
    if (cc->morto) return;

    if (cc->history_count == COPYCAT_DELAY) {
        /* Le o estado mais antigo do historico e aplica ao boss */
        copycat_state_t oldest = cc->history[cc->history_index];
        cc->px = oldest.px;
        cc->py = oldest.py;
        cc->direcao = oldest.direcao;
        cc->frame_atual = oldest.frame_animacao;
    }
}

/* Subtrai a vida do Copycat e ativa os frames de piscar em vermelho */
void copycat_receber_dano(copycat_t *cc, int dano) {
    if (cc->morto || dano <= 0) return;
    
    cc->vida -= dano;
    cc->flash_frames = 4;
    
    if (cc->vida <= 0) {
        cc->vida = 0;
        cc->morto = 1;
    }
}

/* Desenha o Copycat. Se ele estiver morto, desenha invertido. Vivo, tem filtro (tint) roxo */
void copycat_desenhar(const copycat_t *cc, int camera_x, int camera_y) {
    if (!cc->frame_atual) return;
    
    int x = cc->px - camera_x;
    int y = cc->py - camera_y;
    int espelhado = (cc->direcao == -1);

    if (cc->morto) {
        sprite_draw_colorido_flipv(cc->frame_atual, x, y, espelhado, fb_rgb(220, 40, 40));
        return;
    }

    if (cc->flash_frames > 0) {
        sprite_draw_colorido(cc->frame_atual, x, y, espelhado, fb_rgb(255, 40, 40));
    } else {
        /* Aplica o filtro de cor Roxo no sprite original do heroi: fb_rgb(148, 0, 211) */
        sprite_draw_tint(cc->frame_atual, x, y, espelhado, fb_rgb(148, 0, 211), 0.6f);
    }
}

/* Retorna a hitbox do Copycat um pouco menor que o frame original, pra colisao ficar mais justa */
retangulo_t copycat_hitbox(const copycat_t *cc) {
    retangulo_t r;
    r.x = cc->px + 10;
    r.y = cc->py + 10;
    r.largura = 20;
    r.altura = 35;
    return r;
}
