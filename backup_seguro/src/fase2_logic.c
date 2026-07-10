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
#include "inimigos/soldado/soldado.h"
#include "ataques/tiros/tiro.h"
#include "inimigo/inimigo.h"

#include "ui/ui.h"
#include "hardware/hardware_state.h"

#define INIMIGO_INTERVALO_TIRO 180
#define INIMIGO_VIDA   4
#define FASE2_TOTAL_INIMIGOS 20
#define INIMIGOS_SIMULTANEOS  3

typedef struct {
    inimigo_t inimigo;
    int ativo;
} soldado_slot_t;

static void spawnar_soldado(soldado_slot_t *slot, const cenario_t *c, int indice_spawn, tipo_tiro_t tiro) {
    int x = 20 + (rand() % (c->largura - 60));
    int chao = cenario_chao_y(c, x + 38 / 2, 0); // SOLDADO_LARGURA = 38
    inimigo_iniciar(&slot->inimigo, x, c->camera_y - 80, chao - 45, INIMIGO_VIDA,
                     soldado_parado_frames, SOLDADO_PARADO_FRAME_COUNT, 10,
                     soldado_correr_frames, SOLDADO_CORRER_FRAME_COUNT, 5,
                     soldado_atirar_frames, SOLDADO_ATIRAR_FRAME_COUNT, 4,
                     soldado_morrer_frames, SOLDADO_MORRER_FRAME_COUNT, 6,
                     soldado_explosao_frames, SOLDADO_EXPLOSAO_FRAME_COUNT, 4,
                     soldado_paraquedas_frames, SOLDADO_PARAQUEDAS_FRAME_COUNT, 8,
                     tiro, INIMIGO_INTERVALO_TIRO);
    slot->ativo = 1;
}

static void desenhar_cena_fase_1(const cenario_t *c, const jogador_t *j, const tiros_t *tiros,
                                 const soldado_slot_t *soldados, int total_soldados, const tiros_t *tiros_inimigo,
                                 int mira_tela_x, int mira_tela_y, float dx, float dy, int inimigos_restantes, int frame_contador, int fase_terminando) {
    cenario_desenhar_bg(c);
    desenhar_icones(c->camera_x, c->camera_y, frame_contador);
    jogador_desenhar(j, c->camera_x, c->camera_y);
    tiros_desenhar(tiros, c->camera_x, c->camera_y);
    
    for (int i = 0; i < total_soldados; i++) {
        if (soldados[i].ativo) {
            inimigo_desenhar(&soldados[i].inimigo, c->camera_x, c->camera_y);
        }
    }
    
    tiros_desenhar(tiros_inimigo, c->camera_x, c->camera_y);
    cenario_desenhar_fg(c);
    desenhar_mira(mira_tela_x, mira_tela_y, dx, dy, fb_rgb(255, 0, 0));
    desenhar_vida(j->vida, 10);
    desenhar_barras(j->tiros_normais_restantes, j->cooldown_superaquecimento, 
                    j->tiros_carregados_restantes, j->cooldown_recarregar_forte);
    desenhar_numero_2_digitos(inimigos_restantes, FB_WIDTH - 4 - (3 * 2 + 2 + 3 * 2), 4, 2, fb_rgb(255, 255, 255));
    if (fase_terminando) {
        desenhar_sinal_go(FB_WIDTH - 40, 20, frame_contador);
    }
    fb_present();
}

int rodar_fase_2(jogador_t *jogador) {
    cenario_t cenario;
    cenario_iniciar(&cenario, fase2_bg, fase2_fg, fase2_colisao, FASE2_LARGURA, FASE2_ALTURA);

    tiros_t tiros;
    tiros_iniciar(&tiros);

    tiros_t tiros_inimigo;
    tiros_iniciar(&tiros_inimigo);

    tipo_tiro_t tiro_soldado = { &soldado_tiro_frames[0], 3, 1 };
    soldado_slot_t soldados[INIMIGOS_SIMULTANEOS];
    int inimigos_spawnados = 0;
    int inimigos_mortos = 0;

    for (int i = 0; i < INIMIGOS_SIMULTANEOS; i++) {
        spawnar_soldado(&soldados[i], &cenario, inimigos_spawnados, tiro_soldado);
        inimigos_spawnados++;
    }

    for (int i=0; i<MAX_ICONES; i++) {
        icones_mapa[i].ativo = 0;
    }

    int frame_contador = 0;
    int venceu = 0;
    int fase_terminando = 0;
    int fase_entrada = 1;

    while (!fb_poll_quit()) {
        if (hw_jogo_pausado) {
            desenhar_cena_fase_1(&cenario, jogador, &tiros, soldados, INIMIGOS_SIMULTANEOS, &tiros_inimigo, 0, 0, 0, 0, FASE2_TOTAL_INIMIGOS - inimigos_mortos, frame_contador, fase_terminando);
            fb_present();
            continue;
        }

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
        float vx = (float)(mx - mira_tela_x);
        float vy = (float)(my - mira_tela_y);
        float dist = sqrtf(vx * vx + vy * vy);
        dx = (dist > 0.0f) ? vx / dist : 1.0f;
        dy = (dist > 0.0f) ? vy / dist : 0.0f;

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

        for (int i = 0; i < INIMIGOS_SIMULTANEOS; i++) {
            soldado_slot_t *slot = &soldados[i];
            if (!slot->ativo) continue;

            inimigo_atualizar(&slot->inimigo, &tiros_inimigo, centro_mundo_x, centro_mundo_y, &cenario);

            int dano_base = tiros_colidir_com(&tiros, inimigo_hitbox(&slot->inimigo));
            int dano = dano_base * (jogador->buff_instakill > 0 ? 3 : 1);
            inimigo_receber_dano(&slot->inimigo, dano);

            if (inimigo_esta_morto(&slot->inimigo)) {
                if (rand() % 100 < 30) {
                    for (int k=0; k<MAX_ICONES; k++) {
                        if (!icones_mapa[k].ativo) {
                            icones_mapa[k].ativo = 1;
                            int r_tipo = rand() % 100;
                            if (r_tipo < 50) icones_mapa[k].tipo = ICONE_VIDA;
                            else if (r_tipo < 67) icones_mapa[k].tipo = ICONE_VELOCIDADE;
                            else if (r_tipo < 84) icones_mapa[k].tipo = ICONE_INSTAKILL;
                            else icones_mapa[k].tipo = ICONE_SUPER_PULO;
                            retangulo_t h = inimigo_hitbox(&slot->inimigo);
                            icones_mapa[k].px = h.x + h.largura/2 - 8;
                            icones_mapa[k].py = h.y + h.altura - 16;
                            break;
                        }
                    }
                }
                slot->ativo = 0;
                inimigos_mortos++;
                if (inimigos_spawnados < FASE2_TOTAL_INIMIGOS) {
                    spawnar_soldado(slot, &cenario, inimigos_spawnados, tiro_soldado);
                    inimigos_spawnados++;
                }
            }
        }
        tiros_atualizar(&tiros_inimigo);

        int acertos_no_jogador = tiros_colidir_com(&tiros_inimigo, jogador_hitbox(jogador));
        jogador_receber_dano(jogador, acertos_no_jogador);

        int inimigos_restantes = FASE2_TOTAL_INIMIGOS - inimigos_mortos;
        if (inimigos_restantes < 0) inimigos_restantes = 0;
        
        // Atualiza as variaveis do Hardware (DE1-SoC FPGA)
        hw_display_inimigos_restantes = inimigos_restantes;
        hw_leds_vida_personagem = jogador->vida;

        desenhar_cena_fase_1(&cenario, jogador, &tiros, soldados, INIMIGOS_SIMULTANEOS, &tiros_inimigo,
                             mira_tela_x, mira_tela_y, dx, dy, inimigos_restantes, frame_contador, fase_terminando);

        if (jogador->estado == ESTADO_MORRER && animacao_terminou(&jogador->anim_morrer)) {
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
