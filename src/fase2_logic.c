#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "cenario/fases.h"
#include "framebuffer/framebuffer.h"
#include "sprite/sprite.h"
#include "animacao/animacao.h"
#include "colisao/colisao.h"
#include "cenario/cenario.h"
#include "cenario/fase2/fase2.h"

#include "personagem/jogador.h"
#include "ataques/tiros/tiro.h"

/* Spider Jockey — inimigo da Fase 2 */
#include "inimigos/spider_jockey/spider_jockey.h"
#include "inimigos/spider_jockey/spider_jockey_sprites.h"

/* Soldados genéricos ainda são usados como "paraquedistas de apoio"
   (comportamento de paraquedas herdado do sistema original) */
#include "inimigos/soldado/soldado.h"
#include "inimigo/inimigo.h"

#include "ui/ui.h"
#include "hardware/hardware_state.h"

/* ---------------------------------------------------------------- */
/* Constantes da fase                                               */
/* ---------------------------------------------------------------- */
#define FASE2_TOTAL_INIMIGOS    10   /* spider jockeys a matar */
#define SJ_SIMULTANEOS           3   /* spider jockeys no mapa ao mesmo tempo */

/* ---------------------------------------------------------------- */
/* Slot de spider jockey                                            */
/* ---------------------------------------------------------------- */
typedef struct {
    spider_jockey_t sj;
    int ativo;
} sj_slot_t;

/* ---------------------------------------------------------------- */
/* Spawn                                                            */
/* ---------------------------------------------------------------- */
static void spawnar_sj(sj_slot_t *slot, const cenario_t *c) {
    int x    = 20 + (rand() % (c->largura - 60));
    int chao = cenario_chao_y(c, x + SPIDER_JOCKEY_FRAME_WIDTH / 2, 0);

    sj_iniciar(&slot->sj,
               x, c->camera_y - 80, chao - SPIDER_JOCKEY_FRAME_HEIGHT,
               spider_jockey_frames,    2,   /* idle:  frames 0 e 1 */
               spider_jockey_frames,    2,   /* andar: frames 0 e 1 */
               &spider_jockey_frames[2], 1,  /* atirar: frame 2 */
               soldado_paraquedas_frames, SOLDADO_PARAQUEDAS_FRAME_COUNT,
               &spider_jockey_arrow_frames[0]);

    slot->ativo = 1;
}

/* ---------------------------------------------------------------- */
/* Desenho da cena                                                  */
/* ---------------------------------------------------------------- */
static void desenhar_cena_fase_2(const cenario_t *c, const jogador_t *j,
                                  const tiros_t *tiros,
                                  const sj_slot_t *slots, int total_slots,
                                  int mira_tela_x, int mira_tela_y,
                                  float dx, float dy,
                                  int inimigos_restantes, int frame_contador,
                                  int fase_terminando) {
    cenario_desenhar_bg(c);
    desenhar_icones(c->camera_x, c->camera_y, frame_contador);
    jogador_desenhar(j, c->camera_x, c->camera_y);
    tiros_desenhar(tiros, c->camera_x, c->camera_y);

    for (int i = 0; i < total_slots; i++) {
        if (slots[i].ativo) {
            /* sj_desenhar ja inclui as flechas proprias do inimigo */
            sj_desenhar(&slots[i].sj, c->camera_x, c->camera_y);
        }
    }

    cenario_desenhar_fg(c);
    desenhar_mira(mira_tela_x, mira_tela_y, dx, dy, fb_rgb(0, 220, 255));
    desenhar_vida(j->vida, 10);
    desenhar_barras(j->tiros_normais_restantes, j->cooldown_superaquecimento,
                    j->tiros_carregados_restantes, j->cooldown_recarregar_forte);
    desenhar_numero_2_digitos(inimigos_restantes,
                              FB_WIDTH - 4 - (3 * 2 + 2 + 3 * 2), 4, 2,
                              fb_rgb(255, 255, 255));
    if (fase_terminando) {
        desenhar_sinal_go(FB_WIDTH - 40, 20, frame_contador);
    }
    fb_present();
}

/* ---------------------------------------------------------------- */
/* Loop principal da Fase 2                                         */
/* ---------------------------------------------------------------- */
int rodar_fase_2(jogador_t *jogador) {
    cenario_t cenario;
    cenario_iniciar(&cenario, fase2_bg, fase2_fg, fase2_colisao,
                    FASE2_LARGURA, FASE2_ALTURA);

    /* Tiros do heroi */
    tiros_t tiros;
    tiros_iniciar(&tiros);

    /* Spider jockeys */
    sj_slot_t slots[SJ_SIMULTANEOS];
    int inimigos_spawnados = 0;
    int inimigos_mortos    = 0;

    for (int i = 0; i < SJ_SIMULTANEOS; i++) {
        spawnar_sj(&slots[i], &cenario);
        inimigos_spawnados++;
    }

    for (int i = 0; i < MAX_ICONES; i++) {
        icones_mapa[i].ativo = 0;
    }

    int frame_contador  = 0;
    int debug_pass_last = fb_key_down(FB_KEY_DEBUG_PASS);
    int venceu          = 0;
    int fase_terminando = 0;
    int fase_entrada    = 1;

    while (!fb_poll_quit()) {
        int debug_pass_curr = fb_key_down(FB_KEY_DEBUG_PASS);
        if (debug_pass_curr && !debug_pass_last) return 1;
        debug_pass_last = debug_pass_curr;
        
        frame_contador++;
        int fire_clique = 0, fire_forte_clique = 0;

        if (!fase_entrada) {
            jogador_atualizar_entrada_tiro(jogador, &fire_clique, &fire_forte_clique);
        }

        jogador_atualizar(jogador, &cenario, fase_entrada);

        if (fase_entrada && jogador->px >= 50) {
            fase_entrada = 0;
        }

        int centro_mundo_x, centro_mundo_y;
        jogador_centro(jogador, &centro_mundo_x, &centro_mundo_y);
        cenario_centralizar_camera(&cenario, centro_mundo_x, centro_mundo_y);

        int mira_tela_x = centro_mundo_x - cenario.camera_x;
        int mira_tela_y = centro_mundo_y - cenario.camera_y;

        int mx, my;
        fb_mouse_pos(&mx, &my);

        float dx, dy;
        float vx_m = (float)(mx - mira_tela_x);
        float vy_m = (float)(my - mira_tela_y);
        float dist_m = sqrtf(vx_m * vx_m + vy_m * vy_m);
        dx = (dist_m > 0.0f) ? vx_m / dist_m : 1.0f;
        dy = (dist_m > 0.0f) ? vy_m / dist_m : 0.0f;

        if (jogador->estado != ESTADO_MORRER) {
            if (dx < -0.01f) jogador->direcao = -1;
            else if (dx > 0.01f) jogador->direcao = 1;
        }

        jogador_atualizar_animacao(jogador, dy);

        if (!fase_entrada) {
            jogador_processar_tiro(jogador, &tiros, fire_clique, fire_forte_clique,
                                   centro_mundo_x, centro_mundo_y, dx, dy);
        }
        tiros_atualizar(&tiros);

        /* Velocidade do heroi para calculo de dano direcional das flechas.
           vel_x nao esta exposto na jogador_t — usamos 0.0f para horizontal
           (o dano direcional pelo eixo Y ainda funciona via heroi_vy). */
        float heroi_vx = 0.0f;
        float heroi_vy = jogador->vel_y;

        /* --- Atualiza spider jockeys --- */
        for (int i = 0; i < SJ_SIMULTANEOS; i++) {
            sj_slot_t *slot = &slots[i];
            if (!slot->ativo) continue;

            sj_atualizar(&slot->sj,
                         centro_mundo_x, centro_mundo_y,
                         heroi_vx, heroi_vy,
                         &cenario);

            /* Dano dos tiros do heroi no spider jockey */
            int dano_base = tiros_colidir_com(&tiros, sj_hitbox(&slot->sj));
            int dano = dano_base * (jogador->buff_instakill > 0 ? 3 : 1);
            sj_receber_dano(&slot->sj, dano);

            /* Dano das flechas no heroi */
            int dano_flecha = sj_flechas_colidir(&slot->sj,
                                                  jogador_hitbox(jogador),
                                                  heroi_vx, heroi_vy);
            jogador_receber_dano(jogador, dano_flecha);

            /* Morte do spider jockey */
            if (sj_esta_morto(&slot->sj)) {
                /* Drop de item (30% de chance) */
                if (rand() % 100 < 30) {
                    for (int k = 0; k < MAX_ICONES; k++) {
                        if (!icones_mapa[k].ativo) {
                            icones_mapa[k].ativo = 1;
                            int r_tipo = rand() % 100;
                            if      (r_tipo < 50) icones_mapa[k].tipo = ICONE_VIDA;
                            else if (r_tipo < 67) icones_mapa[k].tipo = ICONE_VELOCIDADE;
                            else if (r_tipo < 84) icones_mapa[k].tipo = ICONE_INSTAKILL;
                            else                  icones_mapa[k].tipo = ICONE_SUPER_PULO;
                            retangulo_t h = sj_hitbox(&slot->sj);
                            icones_mapa[k].px = h.x + h.largura  / 2 - 8;
                            icones_mapa[k].py = h.y + h.altura - 16;
                            break;
                        }
                    }
                }

                slot->ativo = 0;
                inimigos_mortos++;

                if (inimigos_spawnados < FASE2_TOTAL_INIMIGOS) {
                    spawnar_sj(slot, &cenario);
                    inimigos_spawnados++;
                }
            }
        }

        /* Atualiza displays / LEDs do hardware */
        int inimigos_restantes = FASE2_TOTAL_INIMIGOS - inimigos_mortos;
        if (inimigos_restantes < 0) inimigos_restantes = 0;
        hw_display_inimigos_restantes = inimigos_restantes;
        hw_leds_vida_personagem       = jogador->vida;

        desenhar_cena_fase_2(&cenario, jogador, &tiros, slots, SJ_SIMULTANEOS,
                             mira_tela_x, mira_tela_y, dx, dy,
                             inimigos_restantes, frame_contador, fase_terminando);

        if (jogador->estado == ESTADO_MORRER &&
            animacao_terminou(&jogador->anim_morrer)) {
            venceu = 0;
            break;
        }

        if (inimigos_mortos >= FASE2_TOTAL_INIMIGOS) {
            fase_terminando = 1;
        }

        if (fase_terminando && jogador->px > cenario.largura - 50) {
            venceu = 1;
            break;
        }
    }

    return venceu;
}
