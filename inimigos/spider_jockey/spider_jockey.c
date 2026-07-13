#include <math.h>
#include <stdlib.h>

#include "inimigos/spider_jockey/spider_jockey.h"
#include "sprite/sprite.h"
#include "framebuffer/framebuffer.h"

/* ================================================================
 * Fisica da flecha parabolica
 *
 * Dado spider em (sx, sy) e alvo em (hx, hy):
 *
 *   dx_mundo = hx - sx           (deslocamento horizontal)
 *   dy_mundo = hy - sy           (positivo = alvo esta ABAIXO)
 *
 * Fixamos um tempo de voo T = SJ_FLECHA_TEMPO_VOO frames.
 * Cinematica sem resistencia do ar:
 *
 *   x(t) = sx + vx0 * t
 *   y(t) = sy + vy0 * t + 0.5 * g * t^2
 *
 * Para acertar o alvo em t = T:
 *   vx0 = dx_mundo / T
 *   vy0 = (dy_mundo - 0.5 * g * T^2) / T
 *
 * O angulo resulta naturalmente da divisao; nao e preciso calcular theta
 * explicitamente. O alcance maximo e limitado pelo SJ_ALCANCE_TIRO.
 *
 * Dano proporcional a velocidade relativa (mecanica Minecraft):
 *   v_rel = dot(vel_heroi - vel_flecha, direcao_flecha)
 *   dano = clamp(BASE + v_rel / ESCALA, BASE, MAX)
 *   Positivo: heroi correndo de encontro a flecha -> mais dano
 *   Negativo: heroi fugindo -> menos dano
 * ================================================================ */

/* ---------------------------------------------------------------- */
/* Helpers internos                                                  */
/* ---------------------------------------------------------------- */

static int sj_clamp(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static void sj_flecha_disparar(spider_jockey_t *sj,
                                int sx, int sy,
                                int alvo_x, int alvo_y) {
    float T   = (float)SJ_FLECHA_TEMPO_VOO;
    float g   = SJ_FLECHA_GRAVIDADE;
    float ddx = (float)(alvo_x - sx);
    float ddy = (float)(alvo_y - sy);

    /* Se o alvo estiver no mesmo pixel, nao dispara (divisao por zero). */
    if (fabsf(ddx) < 1.0f && fabsf(ddy) < 1.0f) return;

    float vx0 = ddx / T;
    float vy0 = (ddy - 0.5f * g * T * T) / T;

    /* Procura slot livre no pool */
    for (int i = 0; i < SJ_FLECHAS_MAX; i++) {
        if (!sj->flechas[i].ativo) {
            sj->flechas[i].x        = (float)sx;
            sj->flechas[i].y        = (float)sy;
            sj->flechas[i].vx       = vx0;
            sj->flechas[i].vy       = vy0;
            sj->flechas[i].ativo    = 1;
            sj->flechas[i].vida     = SJ_FLECHA_VIDA;
            sj->flechas[i].espelhada = (vx0 < 0.0f) ? 1 : 0;
            sj->flechas[i].sprite   = sj->sprite_flecha;
            break;
        }
    }
}

static void sj_flechas_atualizar(spider_jockey_t *sj, const cenario_t *cenario) {
    for (int i = 0; i < SJ_FLECHAS_MAX; i++) {
        sj_flecha_t *f = &sj->flechas[i];
        if (!f->ativo) continue;

        f->vy += SJ_FLECHA_GRAVIDADE;
        f->x  += f->vx;
        f->y  += f->vy;
        f->vida--;

        /* Colisao com o chao/paredes do cenario */
        int cx = (int)f->x + f->sprite->width  / 2;
        int cy = (int)f->y + f->sprite->height / 2;
        int col = cenario_colisao(cenario, cx, (int)f->y + f->sprite->height);
        if (col == CENARIO_SOLIDO) {
            f->ativo = 0;
            continue;
        }

        if (f->vida <= 0) {
            f->ativo = 0;
        }
    }
}

/* ---------------------------------------------------------------- */
/* API publica                                                       */
/* ---------------------------------------------------------------- */

void sj_iniciar(spider_jockey_t *sj,
                int px, int py, int chao_y,
                const sprite_frame_t *idle_frames,       int idle_count,
                const sprite_frame_t *andar_frames,      int andar_count,
                const sprite_frame_t *atirar_frames,     int atirar_count,
                const sprite_frame_t *paraquedas_frames, int paraquedas_count,
                const sprite_frame_t *sprite_flecha) {
    sj->px       = px;
    sj->py       = py;
    sj->vel_y    = 0.0f;
    sj->chao_y   = chao_y;
    sj->direcao  = -1;
    sj->vida     = SJ_VIDA;
    sj->flash_frames         = 0;
    sj->atirando_pose_frames = 0;
    sj->morte_timer          = 0;
    sj->cooldown_tiro        = SJ_INTERVALO_TIRO;
    sj->tempo_decisao        = rand() % 60 + 30;
    sj->comportamento        = rand() % 3;
    sj->estado               = SJ_ESTADO_CAINDO;
    sj->sprite_flecha        = sprite_flecha;

    animacao_iniciar(&sj->anim_idle,       idle_frames,       idle_count,       8, 1);
    animacao_iniciar(&sj->anim_andar,      andar_frames,      andar_count,      4, 1);
    animacao_iniciar(&sj->anim_atirar,     atirar_frames,     atirar_count,     6, 1);
    /* Animacao de morte: usa os frames de idle desenhados de cabeca para baixo */
    animacao_iniciar(&sj->anim_morrer,     idle_frames,       idle_count,       6, 0);
    animacao_iniciar(&sj->anim_paraquedas, paraquedas_frames, paraquedas_count, 8, 1);

    sj->frame = animacao_frame_atual(&sj->anim_paraquedas);

    for (int i = 0; i < SJ_FLECHAS_MAX; i++) {
        sj->flechas[i].ativo = 0;
    }
}

void sj_atualizar(spider_jockey_t *sj,
                  int alvo_x, int alvo_y,
                  float alvo_vx, float alvo_vy,
                  const cenario_t *cenario) {

    /* --- Flash de dano --- */
    if (sj->flash_frames > 0) sj->flash_frames--;

    /* --- Estado de morte --- */
    if (sj->estado == SJ_ESTADO_MORRENDO) {
        sj->morte_timer++;
        animacao_atualizar(&sj->anim_morrer);
        sj->frame = animacao_frame_atual(&sj->anim_morrer);
        sj_flechas_atualizar(sj, cenario);
        return;
    }

    /* --- Fisica de queda / paraquedas (igual ao soldado) --- */
    if (sj->estado == SJ_ESTADO_CAINDO) {
        sj->vel_y = 1.5f;
        sj->py   += (int)sj->vel_y;

        int chao_atual = cenario_chao_y(cenario,
                                        sj->px + sj->frame->width  / 2,
                                        sj->py + sj->frame->height / 2);
        if (sj->py >= chao_atual - sj->frame->height) {
            int tipo = cenario_colisao(cenario,
                                       sj->px + sj->frame->width / 2,
                                       chao_atual);
            if (tipo == CENARIO_PLATAFORMA && sj->comportamento == 1) {
                sj->py = chao_atual + 2; /* atravessa plataforma */
            } else {
                sj->py    = chao_atual - sj->frame->height;
                sj->estado = SJ_ESTADO_CHAO;
            }
        }
        animacao_atualizar(&sj->anim_paraquedas);
        /* Atualiza frame do CORPO para que o esqueleto seja visivel sob o paraquedas */
        animacao_atualizar(&sj->anim_idle);
        sj->frame = animacao_frame_atual(&sj->anim_idle);

        /* Cooldown de tiro funciona mesmo caindo */
        if (sj->cooldown_tiro > 0) sj->cooldown_tiro--;

        /* Dispara enquanto cai se estiver no alcance */
        if (sj->cooldown_tiro == 0) {
            int cx2 = sj->px + sj->frame->width  / 2;
            int cy2 = sj->py + sj->frame->height / 2;
            float dvx = (float)(alvo_x - cx2);
            float dvy = (float)(alvo_y - cy2);
            float dd  = sqrtf(dvx * dvx + dvy * dvy);
            if (dd < (float)SJ_ALCANCE_TIRO) {
                sj_flecha_disparar(sj, cx2, cy2, alvo_x, alvo_y);
                sj->cooldown_tiro        = SJ_INTERVALO_TIRO;
                sj->atirando_pose_frames = SJ_POSE_ATIRAR_FRAMES;
            }
        }

        sj_flechas_atualizar(sj, cenario);
        return;
    }

    /* --- Fisica de pulo --- */
    if (sj->estado == SJ_ESTADO_PULANDO) {
        sj->vel_y += 0.5f;
        sj->py    += (int)sj->vel_y;
        sj->px    += sj->direcao * SJ_VELOCIDADE;

        if (sj->vel_y > 0.0f) {
            int chao_atual = cenario_chao_y(cenario,
                                            sj->px + sj->frame->width  / 2,
                                            sj->py + sj->frame->height / 2);
            if (sj->py >= chao_atual - sj->frame->height) {
                sj->py    = chao_atual - sj->frame->height;
                sj->estado = SJ_ESTADO_CHAO;
            }
        }
    }

    /* --- Cooldown de tiro --- */
    if (sj->cooldown_tiro > 0) sj->cooldown_tiro--;

    /* --- Vetores para o alvo --- */
    int   centro_x = sj->px + sj->frame->width  / 2;
    int   centro_y = sj->py + sj->frame->height / 2;
    float vx       = (float)(alvo_x - centro_x);
    float vy       = (float)(alvo_y - centro_y);
    float dist     = sqrtf(vx * vx + vy * vy);

    /* --- IA de decisao --- */
    if (sj->tempo_decisao > 0) {
        sj->tempo_decisao--;
    } else {
        sj->tempo_decisao = rand() % 90 + 40;
        sj->comportamento = rand() % 3;
    }

    int andando = 0;

    if (sj->estado == SJ_ESTADO_CHAO && sj->atirando_pose_frames == 0) {

        if (sj->comportamento == 0 && dist > SJ_ALCANCE_PATRULHA * 0.3f) {
            /* Seguir — mais rapido e alcance maior que o soldado */
            sj->direcao = (vx < 0.0f) ? -1 : 1;
            sj->px     += sj->direcao * SJ_VELOCIDADE;
            andando     = 1;

            /* Pulo para atingir alvos em plataformas acima */
            if (vy < -40.0f && (rand() % 50 == 0)) {
                sj->estado = SJ_ESTADO_PULANDO;
                sj->vel_y  = -9.0f;
                andando    = 0;
            } else if (vy > 60.0f && (rand() % 50 == 0)) {
                sj->estado = SJ_ESTADO_PULANDO;
                sj->vel_y  = 1.0f;
                sj->py    += 15;
                andando    = 0;
            }

        } else if (sj->comportamento == 2 && dist < SJ_ALCANCE_PATRULHA) {
            /* Afastar — usa velocidade SJ_VELOCIDADE tambem */
            sj->direcao = (vx < 0.0f) ? 1 : -1;
            sj->px     += sj->direcao * SJ_VELOCIDADE;
            andando     = 1;
        } else {
            /* Parado — vira para o alvo */
            if (dist > 0.0f) {
                sj->direcao = (vx < 0.0f) ? -1 : 1;
            }
        }
    }

    /* Sempre vira para o alvo quando nao esta se movendo */
    if (dist > 0.0f && !andando) {
        sj->direcao = (vx < 0.0f) ? -1 : 1;
    }

    /* --- Disparo parabolico --- */
    if (sj->cooldown_tiro == 0 && dist < (float)SJ_ALCANCE_TIRO && sj->estado == SJ_ESTADO_CHAO) {
        sj_flecha_disparar(sj, centro_x, centro_y, alvo_x, alvo_y);
        sj->cooldown_tiro        = SJ_INTERVALO_TIRO;
        sj->atirando_pose_frames = SJ_POSE_ATIRAR_FRAMES;
    }

    /* --- Colisao com o chao ao andar --- */
    if (andando) {
        int chao_atual = cenario_chao_y(cenario,
                                        sj->px + sj->frame->width  / 2,
                                        sj->py + sj->frame->height / 2);
        sj->py = chao_atual - sj->frame->height;
    }

    /* --- Animacao ---
     * Frames 0-1 = idle/andar (anim_idle e anim_andar tem frame_count=2)
     * Frame 2    = atacando (anim_atirar tem frame_count=1, aponta para frame 2) */
    animacao_t *anim_atual;
    if (sj->estado == SJ_ESTADO_PULANDO) {
        anim_atual = &sj->anim_idle;
    } else if (andando) {
        anim_atual = &sj->anim_andar;
    } else if (sj->atirando_pose_frames > 0) {
        anim_atual = &sj->anim_atirar;
        sj->atirando_pose_frames--;
    } else {
        anim_atual = &sj->anim_idle;
    }

    animacao_atualizar(anim_atual);
    sj->frame = animacao_frame_atual(anim_atual);

    /* --- Flechas --- */
    sj_flechas_atualizar(sj, cenario);
}

void sj_desenhar(const spider_jockey_t *sj, int camera_x, int camera_y) {
    int x = sj->px - camera_x;
    int y = sj->py - camera_y;

    /* Spider jockey: sprite original esta virado para a direita.
       Espelha se direcao == -1 (esquerda). */
    int espelhado = (sj->direcao == -1);

    if (sj->estado == SJ_ESTADO_MORRENDO) {
        /* Morte: sprite de cabeca para baixo (flip vertical) + tint vermelho. */
        sprite_draw_colorido_flipv(sj->frame, x, y, espelhado, fb_rgb(220, 40, 40));
        return;
    }

    if (sj->flash_frames > 0) {
        sprite_draw_colorido(sj->frame, x, y, espelhado, fb_rgb(255, 40, 40));
    } else {
        sprite_draw(sj->frame, x, y, espelhado);
    }

    /* Sem paraquedas, ele cai direto do teto. */

    /* Flechas */
    for (int i = 0; i < SJ_FLECHAS_MAX; i++) {
        const sj_flecha_t *f = &sj->flechas[i];
        if (!f->ativo) continue;
        int fx = (int)f->x - camera_x;
        int fy = (int)f->y - camera_y;
        sprite_draw(f->sprite, fx, fy, f->espelhada);
    }
}

retangulo_t sj_hitbox(const spider_jockey_t *sj) {
    return (retangulo_t){ sj->px, sj->py, sj->frame->width, sj->frame->height };
}

int sj_flechas_colidir(spider_jockey_t *sj,
                        retangulo_t alvo,
                        float alvo_vx, float alvo_vy) {
    int dano_total = 0;

    for (int i = 0; i < SJ_FLECHAS_MAX; i++) {
        sj_flecha_t *f = &sj->flechas[i];
        if (!f->ativo) continue;

        retangulo_t hitbox_flecha = {
            (int)f->x, (int)f->y,
            f->sprite->width, f->sprite->height
        };

        if (!colisao_retangulos(hitbox_flecha, alvo)) continue;

        /* --- Mecanica de dano direcional (Minecraft-style) ---
         *
         * A "ponta" da flecha e o lado no sentido de vx (direita se vx > 0).
         * Verificamos se o heroi esta sendo atingido pelo lado da ponta:
         *   - Centro do heroi em relacao ao centro da flecha
         *   - Se o heroi esta do lado para onde a flecha aponta -> dano valido
         *
         * Velocidade relativa: v_rel = dot(vel_heroi - vel_flecha, dir_flecha)
         *   v_rel > 0: heroi vem em direcao a flecha  -> mais dano
         *   v_rel < 0: heroi foge da flecha            -> menos dano
         *   v_rel = 0: parado                          -> dano base
         */
        float dir_x = (f->vx > 0.0f) ? 1.0f : -1.0f;

        /* Lado do heroi em relacao ao centro da flecha */
        float heroi_cx = (float)(alvo.x + alvo.largura  / 2);
        float flecha_cx= f->x + f->sprite->width / 2.0f;
        float lado     = (heroi_cx - flecha_cx) * dir_x;

        /* So causa dano se o heroi estiver do lado da ponta */
        if (lado < -4.0f) {
            /* Acertou pelo lado do cabo — apenas desativa a flecha, sem dano */
            f->ativo = 0;
            continue;
        }

        /* Velocidade relativa projetada na direcao da flecha */
        float v_rel = (alvo_vx - f->vx) * dir_x + (alvo_vy - f->vy) * 0.0f;
        /* Nota: componente Y da direcao e 0 para simplificar — a flecha e horizontal
           para efeitos de dano direcional; a parabolica e na fisica, nao no calculo de dano. */

        int dano = SJ_FLECHA_DANO_BASE + (int)(v_rel / SJ_FLECHA_DANO_ESCALA);
        dano = sj_clamp(dano, SJ_FLECHA_DANO_BASE, SJ_FLECHA_DANO_MAX);

        dano_total += dano;
        f->ativo = 0;
    }

    return dano_total;
}

void sj_receber_dano(spider_jockey_t *sj, int dano) {
    if (sj->estado == SJ_ESTADO_MORRENDO || dano <= 0) return;

    sj->vida        -= dano;
    sj->flash_frames = SJ_FLASH_FRAMES;

    if (sj->vida <= 0) {
        sj->vida        = 0;
        sj->estado      = SJ_ESTADO_MORRENDO;
        sj->morte_timer = 0;
        animacao_reiniciar(&sj->anim_morrer);
    }
}

int sj_esta_morto(const spider_jockey_t *sj) {
    return sj->estado == SJ_ESTADO_MORRENDO &&
           sj->morte_timer >= SJ_MORTE_FRAMES;
}
