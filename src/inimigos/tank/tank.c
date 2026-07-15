/* Utilidade: Comportamento do Tank: maquina que atira Goombas no ar */
#include "inimigos/tank/tank.h"
#include <string.h>
#include <stdlib.h>

/* Inicializa a struct do Tanque, carregando as animacoes de parado (idle) e tiro */
void tank_iniciar(tank_t *t, int px, int py) {
    memset(t, 0, sizeof(tank_t));
    t->px = px;
    t->py = py;
    t->estado = TANK_IDLE;
    
    animacao_iniciar(&t->anim_idle, tank_idle_frames, TANK_IDLE_FRAME_COUNT, 20, 1);
    animacao_iniciar(&t->anim_shooting, tank_shooting_frames, TANK_SHOOTING_FRAME_COUNT, 8, 0); /* Nao repete (sem loop) */
    t->frame_death = &tank_death_frames[0];
    t->timer = 0;
}

/* Atualiza a IA do Tanque: alterna entre idle e tiro. O tiro dele lança um Goomba pelo ar! */
void tank_atualizar(tank_t *t, goomba_t *goombas, int num_goombas, int jogador_x) {
    if (t->estado == TANK_DEATH) return;

    if (t->estado == TANK_IDLE) {
        animacao_atualizar(&t->anim_idle);
        t->timer++;
        
        /* Apos certo tempo parado (idle), inicia a animacao de disparo */
        if (t->timer > 20) { /* ~300ms de intervalo (20 frames) */
            t->estado = TANK_SHOOTING;
            t->atirou_neste_ciclo = 0;
            animacao_iniciar(&t->anim_shooting, tank_shooting_frames, TANK_SHOOTING_FRAME_COUNT, 25, 0);
        }
    } 
    else if (t->estado == TANK_SHOOTING) {
        animacao_atualizar(&t->anim_shooting);
        
        /* O tiro sai exatamente nos frames 9 ou 10 da animacao do canhao disparando */
        if ((t->anim_shooting.frame_atual == 9 || t->anim_shooting.frame_atual == 10) && !t->atirou_neste_ciclo) {
            /* Procura um Goomba inativo no pool de inimigos para usa-lo como projetil */
            for (int i = 0; i < num_goombas; i++) {
                if (goombas[i].estado == GOOMBA_INATIVO) {
                    float launch_vx = -10.0f - ((rand() % 20) / 10.0f); /* Lanca pra cima e pra tras */
                    float launch_vy = -10.0f - ((rand() % 20) / 10.0f); /* Parabola acentuada */
                    
                    int tiro_x = t->px - TANK_SHOOTING_FRAME_WIDTH + 60;
                    int tiro_y = t->py - (TANK_SHOOTING_FRAME_HEIGHT * 2) + 80;
                    
                    goomba_iniciar(&goombas[i], (float)tiro_x, (float)tiro_y, launch_vx, launch_vy);
                    t->atirou_neste_ciclo = 1;
                    break;
                }
            }
        }
        
        if (animacao_terminou(&t->anim_shooting)) {
            t->estado = TANK_IDLE;
            t->timer = 0;
            animacao_iniciar(&t->anim_idle, tank_idle_frames, TANK_IDLE_FRAME_COUNT, 20, 1);
        }
    }
}

/* Desenha o Tanque ampliado 2x na tela, e ajusta a posicao (px,py) que eh ancorada pelo pe/direito */
void tank_desenhar(const tank_t *t, int camera_x, int camera_y) {
    int draw_x = t->px - camera_x; /* px = canto inferior direito */
    int draw_y = t->py - camera_y; /* py = colisao com o chao (pe) */
    
    /* As sprites originais do tanque ja apontam para a esquerda, logo espelhado = 0 */
    int espelhado = 0;
    
    const sprite_frame_t *f = NULL;
    
    if (t->estado == TANK_IDLE) {
        f = animacao_frame_atual(&t->anim_idle);
    } else if (t->estado == TANK_SHOOTING) {
        f = animacao_frame_atual(&t->anim_shooting);
    } else if (t->estado == TANK_DEATH) {
        f = t->frame_death;
    }
    
    if (f) {
        sprite_draw_escala(f, draw_x - f->width, draw_y - f->height * 2, espelhado, 2);
    }
}
