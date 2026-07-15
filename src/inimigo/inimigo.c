/* Utilidade: Fisica basica e maquina de estados comum (andar, atirar, cair) para todos inimigos */
#include <math.h>
#include <stdlib.h>
#include "inimigo/inimigo.h"

/* quantos frames de jogo a pose de atirar fica visivel depois de um disparo */
#define INIMIGO_POSE_ATIRAR_FRAMES 12
/* quantos frames de jogo a silhueta fica vermelha depois de tomar um hit */
#define INIMIGO_FLASH_FRAMES 8

/* Inicializa a struct do inimigo: seta os frames das animacoes, vida, cooldown do tiro, e o tipo de projetil */
void inimigo_iniciar(inimigo_t *inimigo, int px, int py, int chao_y, int vida,
                      const sprite_frame_t *idle_frames, int idle_frame_count, int idle_frames_por_sprite,
                      const sprite_frame_t *andar_frames, int andar_frame_count, int andar_frames_por_sprite,
                      const sprite_frame_t *atirar_frames, int atirar_frame_count, int atirar_frames_por_sprite,
                      const sprite_frame_t *morrer_frames, int morrer_frame_count, int morrer_frames_por_sprite,
                      const sprite_frame_t *explosao_frames, int explosao_frame_count, int explosao_frames_por_sprite,
                      const sprite_frame_t *paraquedas_frames, int paraquedas_frame_count, int paraquedas_frames_por_sprite,
                      tipo_tiro_t tiro, int intervalo_tiro) {
    inimigo->px = px;
    inimigo->py = py;
    inimigo->vel_y = 0.0f;
    inimigo->chao_y = chao_y;
    inimigo->direcao = -1;

    animacao_iniciar(&inimigo->anim_idle,   idle_frames,   idle_frame_count,   idle_frames_por_sprite,   1);
    animacao_iniciar(&inimigo->anim_andar,  andar_frames,  andar_frame_count,  andar_frames_por_sprite,  1);
    animacao_iniciar(&inimigo->anim_atirar, atirar_frames, atirar_frame_count, atirar_frames_por_sprite, 1);
    animacao_iniciar(&inimigo->anim_morrer, morrer_frames, morrer_frame_count, morrer_frames_por_sprite, 0);
    animacao_iniciar(&inimigo->anim_explosao, explosao_frames, explosao_frame_count, explosao_frames_por_sprite, 0);
    animacao_iniciar(&inimigo->anim_paraquedas, paraquedas_frames, paraquedas_frame_count, paraquedas_frames_por_sprite, 1);

    inimigo->estado = INIMIGO_ESTADO_CAINDO;
    inimigo->frame = animacao_frame_atual(&inimigo->anim_paraquedas);
    inimigo->atirando_pose_frames = 0;

    inimigo->tempo_decisao = rand() % 60 + 30;
    inimigo->comportamento = rand() % 3;

    inimigo->tiro = tiro;
    inimigo->intervalo_tiro = intervalo_tiro;
    inimigo->cooldown_tiro = intervalo_tiro;

    inimigo->vida = vida;
    inimigo->flash_frames = 0;
}

/* IA principal do inimigo: calcula a distancia pro jogador e decide se vai andar, atirar ou pular */
void inimigo_atualizar(inimigo_t *inimigo, tiros_t *tiros, int alvo_x, int alvo_y, const cenario_t *cenario) {
    if (inimigo->flash_frames > 0) {
        inimigo->flash_frames--;
    }

    if (inimigo->estado == INIMIGO_ESTADO_MORRENDO) {
        animacao_atualizar(&inimigo->anim_morrer);
        inimigo->frame = animacao_frame_atual(&inimigo->anim_morrer);
        if (animacao_terminou(&inimigo->anim_morrer)) {
            inimigo->estado = INIMIGO_ESTADO_EXPLODINDO;
            animacao_reiniciar(&inimigo->anim_explosao);
        }
        return;
    } else if (inimigo->estado == INIMIGO_ESTADO_EXPLODINDO) {
        animacao_atualizar(&inimigo->anim_explosao);
        inimigo->frame = animacao_frame_atual(&inimigo->anim_explosao);
        return;
    }

    if (inimigo->estado == INIMIGO_ESTADO_CAINDO) {
        inimigo->vel_y = 1.5f; /* Cai de paraquedas suavemente */
        inimigo->py += (int)inimigo->vel_y;
        
        int chao_atual = cenario_chao_y(cenario, inimigo->px + inimigo->frame->width / 2, inimigo->py + inimigo->frame->height / 2);
        if (inimigo->py >= chao_atual - inimigo->frame->height) {
            int tipo = cenario_colisao(cenario, inimigo->px + inimigo->frame->width / 2, chao_atual);
            /* Tem chance de ignorar plataformas e cair ate o chao de verdade */
            if (tipo == CENARIO_PLATAFORMA && inimigo->comportamento == 1) {
                inimigo->py = chao_atual + 2;
            } else {
                inimigo->py = chao_atual - inimigo->frame->height;
                inimigo->estado = INIMIGO_ESTADO_CHAO;
            }
        }

        animacao_atualizar(&inimigo->anim_paraquedas);
    } else if (inimigo->estado == INIMIGO_ESTADO_PULANDO) {
        inimigo->vel_y += 0.5f; /* Gravidade */
        inimigo->py += (int)inimigo->vel_y;
        inimigo->px += inimigo->direcao * 1;
        
        if (inimigo->vel_y > 0.0f) {
            int chao_atual = cenario_chao_y(cenario, inimigo->px + inimigo->frame->width / 2, inimigo->py + inimigo->frame->height / 2);
            if (inimigo->py >= chao_atual - inimigo->frame->height) {
                inimigo->py = chao_atual - inimigo->frame->height;
                inimigo->estado = INIMIGO_ESTADO_CHAO;
            }
        }
    }
    if (inimigo->cooldown_tiro > 0) {
        inimigo->cooldown_tiro--;
    }

    int centro_x = inimigo->px + inimigo->frame->width / 2;
    int centro_y = inimigo->py + inimigo->frame->height / 2;
    float vx = (float)(alvo_x - centro_x);
    float vy = (float)(alvo_y - centro_y);
    float dist = sqrtf(vx * vx + vy * vy);
    
    int andando = 0;

    if (inimigo->tempo_decisao > 0) {
        inimigo->tempo_decisao--;
    } else {
        inimigo->tempo_decisao = rand() % 90 + 30; /* Escolhe o que fazer nos proximos 30-120 frames */
        inimigo->comportamento = rand() % 3; /* 0 = seguir, 1 = ficar parado, 2 = fugir (ou afastar) */
    }

    /* IA de comportamento */
    if (inimigo->estado == INIMIGO_ESTADO_CHAO && inimigo->atirando_pose_frames == 0) {
        if (inimigo->comportamento == 0 && dist > 80.0f) {
            /* Seguir */
            inimigo->direcao = (vx < 0.0f) ? -1 : 1;
            inimigo->px += inimigo->direcao * 1;
            andando = 1;
            
            if (vy < -40.0f && (rand() % 60 == 0)) {
                /* Player is above, jump */
                inimigo->estado = INIMIGO_ESTADO_PULANDO;
                inimigo->vel_y = -8.5f;
                andando = 0;
            } else if (vy > 60.0f && (rand() % 60 == 0)) {
                /* Player is below, drop down through platform */
                inimigo->estado = INIMIGO_ESTADO_PULANDO;
                inimigo->vel_y = 1.0f;
                inimigo->py += 15; /* Push them through the platform surface */
                andando = 0;
            }
        } else if (inimigo->comportamento == 2 && dist < 250.0f) {
            /* Afastar */
            inimigo->direcao = (vx < 0.0f) ? 1 : -1;
            inimigo->px += inimigo->direcao * 1;
            andando = 1;
        } else {
            /* Ficar parado e olhar para o player */
            if (dist > 0.0f) {
                inimigo->direcao = (vx < 0.0f) ? -1 : 1;
            }
        }
    }

    /* Atira independente do comportamento se estiver virado e cooldown for zero */
    if (dist > 0.0f && !andando) {
        inimigo->direcao = (vx < 0.0f) ? -1 : 1;
    }

    if (inimigo->cooldown_tiro == 0 && dist < 250.0f) {
        float dx = (dist > 0.0f) ? vx / dist : (float)inimigo->direcao;
        float dy = (dist > 0.0f) ? vy / dist : 0.0f;

        tiros_disparar(tiros, centro_x, centro_y, dx, dy, &inimigo->tiro);
        inimigo->cooldown_tiro = inimigo->intervalo_tiro;
        inimigo->atirando_pose_frames = INIMIGO_POSE_ATIRAR_FRAMES;
    }

    if (andando) {
        int chao_atual = cenario_chao_y(cenario, centro_x, inimigo->py + inimigo->frame->height / 2);
        inimigo->py = chao_atual - inimigo->frame->height;
    }

    animacao_t *anim_atual;
    if (inimigo->estado == INIMIGO_ESTADO_MORRENDO) {
        anim_atual = &inimigo->anim_morrer;
    } else if (inimigo->estado == INIMIGO_ESTADO_EXPLODINDO) {
        anim_atual = &inimigo->anim_explosao;
    } else if (inimigo->estado == INIMIGO_ESTADO_CAINDO) {
        anim_atual = &inimigo->anim_idle;
    } else if (inimigo->estado == INIMIGO_ESTADO_PULANDO) {
        anim_atual = &inimigo->anim_idle;
    } else {
        if (andando) {
            anim_atual = &inimigo->anim_andar;
        } else if (inimigo->atirando_pose_frames > 0) {
            anim_atual = &inimigo->anim_atirar;
            inimigo->atirando_pose_frames--;
        } else {
            anim_atual = &inimigo->anim_idle;
        }
    }
    /* Nao deixa o inimigo sair pelas laterais do cenario */
    if (inimigo->px < 0) {
        inimigo->px = 0;
    } else if (inimigo->px > cenario->largura - inimigo->frame->width) {
        inimigo->px = cenario->largura - inimigo->frame->width;
    }
    
    animacao_atualizar(anim_atual);
    inimigo->frame = animacao_frame_atual(anim_atual);
}

/* Desenha o inimigo com base na camera. Se tomou tiro ha pouco tempo, pisca na cor vermelha */
void inimigo_desenhar(const inimigo_t *inimigo, int camera_x, int camera_y) {
    int x = inimigo->px - camera_x;
    int y = inimigo->py - camera_y;
    
    /* A sprite base esta virada para a esquerda. 
       Portanto, se direcao == 1 (direita), precisamos espelhar (1).
       Se direcao == -1 (esquerda), espelhar = 0. */
    int espelhado = inimigo->direcao == 1;

    if (inimigo->flash_frames > 0 && inimigo->estado != INIMIGO_ESTADO_MORRENDO && inimigo->estado != INIMIGO_ESTADO_EXPLODINDO) {
        sprite_draw_colorido(inimigo->frame, x, y, espelhado, fb_rgb(255, 40, 40));
    } else {
        sprite_draw(inimigo->frame, x, y, espelhado);
    }
    
    if (inimigo->estado == INIMIGO_ESTADO_CAINDO) {
        const sprite_frame_t *pq = animacao_frame_atual((animacao_t*)&inimigo->anim_paraquedas);
        sprite_draw(pq, x, y - pq->height + 10, espelhado);
    }
}

/* Retorna a "caixa" (AABB) exata onde o inimigo esta no mapa para calcular tiros batendo nele */
retangulo_t inimigo_hitbox(const inimigo_t *inimigo) {
    return (retangulo_t){ inimigo->px, inimigo->py, inimigo->frame->width, inimigo->frame->height };
}

/* Reduz a vida do inimigo, da o trigger do flash vermelho, e muda o estado pra morrer se a vida zerar */
void inimigo_receber_dano(inimigo_t *inimigo, int dano) {
    if (inimigo->estado == INIMIGO_ESTADO_MORRENDO || inimigo->estado == INIMIGO_ESTADO_EXPLODINDO || dano <= 0) return;

    inimigo->vida -= dano;
    inimigo->flash_frames = INIMIGO_FLASH_FRAMES;

    if (inimigo->vida <= 0) {
        inimigo->vida = 0;
        inimigo->estado = INIMIGO_ESTADO_MORRENDO;
        animacao_reiniciar(&inimigo->anim_morrer);
    }
}

/* Confirma se a animacao de morte/explosao finalizou, avisando a maquina que o inimigo ja pode sumir */
int inimigo_esta_morto(const inimigo_t *inimigo) {
    return inimigo->estado == INIMIGO_ESTADO_EXPLODINDO && animacao_terminou(&inimigo->anim_explosao);
}
