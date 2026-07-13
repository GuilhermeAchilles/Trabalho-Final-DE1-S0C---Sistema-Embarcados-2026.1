#include "inimigos/goomba/goomba.h"
#include "inimigos/goomba/goomba_anm.h"
#include <stdlib.h>

void goomba_iniciar(goomba_t *g, float x, float y, float vx, float vy) {
    g->x = x;
    g->y = y;
    g->vx = vx;
    g->vy = vy;
    g->direcao = -1; // Comeca indo pra esquerda
    g->estado = GOOMBA_VOANDO;
    g->timer_morto = 0;
    
    animacao_iniciar(&g->anim_andar, goomba_frames, 4, 15, 1); // frames 0..3
}

void goomba_atualizar(goomba_t *g, const cenario_t *cenario) {
    if (g->estado == GOOMBA_INATIVO) return;
    
    if (g->estado == GOOMBA_ESMAGADO) {
        g->timer_morto++;
        if (g->timer_morto > 60) {
            g->estado = GOOMBA_INATIVO;
        }
        return;
    }
    
    // Gravidade
    g->vy += 0.2f;
    if (g->vy > 8.0f) g->vy = 8.0f;
    
    // Movimento X
    int move_x = 1;
    if (g->estado == GOOMBA_VOANDO) {
        // No ar, mantem o vx do tiro
        float next_x = g->x + g->vx;
        if (!cenario_colisao(cenario, (int)next_x, (int)(g->y - GOOMBA_FRAME_HEIGHT/2)) &&
            !cenario_colisao(cenario, (int)next_x, (int)g->y)) {
            g->x = next_x;
        } else {
            g->vx = -g->vx; // rebate na parede
        }
    } else if (g->estado == GOOMBA_ANDANDO || g->estado == GOOMBA_PULANDO) {
        // Andando normal
        g->vx = g->direcao * 1.5f;
        float next_x = g->x + g->vx;
        
        // Verifica colisao lateral
        int pe_y = (int)g->y - 2;
        int centro_y = (int)g->y - GOOMBA_FRAME_HEIGHT/2;
        int check_x = g->direcao == 1 ? (int)next_x + GOOMBA_FRAME_WIDTH/2 : (int)next_x - GOOMBA_FRAME_WIDTH/2;
        
        if (cenario_colisao(cenario, check_x, pe_y) || cenario_colisao(cenario, check_x, centro_y)) {
            g->direcao = -g->direcao;
        } else {
            g->x = next_x;
        }
        
        // Pulo esporadico (apenas no chao)
        if (g->estado == GOOMBA_ANDANDO && rand() % 1000 < 10) {
            g->vy = -2.5f;
            g->estado = GOOMBA_PULANDO;
        }
    }
    
    // Movimento Y
    float next_y = g->y + g->vy;
    int chao = cenario_chao_y(cenario, (int)g->x, (int)(g->y - 10));
    
    if (next_y >= chao) {
        g->y = (float)chao;
        g->vy = 0;
        if (g->estado == GOOMBA_VOANDO || g->estado == GOOMBA_PULANDO) {
            g->estado = GOOMBA_ANDANDO;
        }
    } else {
        g->y = next_y;
    }
    
    animacao_atualizar(&g->anim_andar);
}

void goomba_desenhar(const goomba_t *g, int camera_x, int camera_y) {
    if (g->estado == GOOMBA_INATIVO) return;
    
    int draw_x = (int)g->x - GOOMBA_FRAME_WIDTH/2 - camera_x;
    int draw_y = (int)g->y - GOOMBA_FRAME_HEIGHT - camera_y;
    int espelhado = (g->direcao == 1);
    
    if (g->estado == GOOMBA_ESMAGADO) {
        sprite_draw(&goomba_frames[4], draw_x, draw_y, espelhado);
    } else {
        const sprite_frame_t *frame = animacao_frame_atual(&g->anim_andar);
        sprite_draw(frame, draw_x, draw_y, espelhado);
    }
}

void goomba_esmagar(goomba_t *g) {
    if (g->estado != GOOMBA_ANDANDO && g->estado != GOOMBA_PULANDO) return;
    g->estado = GOOMBA_ESMAGADO;
    g->timer_morto = 0;
}

retangulo_t goomba_hitbox(const goomba_t *g) {
    retangulo_t r;
    r.largura = GOOMBA_FRAME_WIDTH - 4;
    r.altura = GOOMBA_FRAME_HEIGHT - 2;
    r.x = (int)g->x - r.largura/2;
    r.y = (int)g->y - r.altura;
    return r;
}
