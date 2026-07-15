/* Utilidade: Cutscene interativa e mecanicas da Fase 1.5 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "framebuffer/framebuffer.h"
#include "cenario/cenario.h"
#include "cenario/fase1_5/fase1_5.h"
#include "personagem/jogador.h"
#include "ataques/tiros/tiro.h"
#include "inimigos/copycat/copycat.h"
#include "ui/ui.h"
#include "hardware/hardware_state.h"

static void desenhar_cena_fase_1_5(const cenario_t *c, const jogador_t *j,
                                   const tiros_t *tiros_jogador, const tiros_t *tiros_boss,
                                   const copycat_t *cc,
                                   int mira_tela_x, int mira_tela_y,
                                   float dx, float dy,
                                   int inimigos_restantes, int frame_contador,
                                   int fase_terminando) {
    cenario_desenhar_bg(c);
    /* Nao ha icones dropados na fase do boss */
    jogador_desenhar(j, c->camera_x, c->camera_y);
    tiros_desenhar(tiros_jogador, c->camera_x, c->camera_y);
    tiros_desenhar_tint(tiros_boss, c->camera_x, c->camera_y, fb_rgb(148, 0, 211), 0.6f);

    if (!cc->morto || cc->flash_frames > 0) {
        copycat_desenhar(cc, c->camera_x, c->camera_y);
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

int rodar_fase_1_5(jogador_t *jogador) {
    cenario_t cenario;
    cenario_iniciar(&cenario, fase1_5_bg, NULL, fase1_5_colisao,
                    FASE1_5_LARGURA, FASE1_5_ALTURA);

    tiros_t tiros_jogador;
    tiros_iniciar(&tiros_jogador);
    
    tiros_t tiros_boss;
    tiros_iniciar(&tiros_boss);

    copycat_t copycat;
    /* Spawna o boss no lado direito da tela */
    copycat_iniciar(&copycat, 250, 160);

    int frame_contador = 0;
    int debug_pass_last = fb_key_down(FB_KEY_DEBUG_PASS);
    int venceu = 0;
    int fase_terminando = 0;
    int fase_entrada = 1;

    /* Define a posicao inicial do jogador */
    jogador->px = 50;

    while (!fb_poll_quit()) {
        int debug_pass_curr = fb_key_down(FB_KEY_DEBUG_PASS);
        if (debug_pass_curr && !debug_pass_last) return 1;
        debug_pass_last = debug_pass_curr;
        
        frame_contador++;
        int fire_clique = 0, fire_forte_clique = 0;

        if (!fase_entrada) {
            jogador_atualizar_entrada_tiro(jogador, &fire_clique, &fire_forte_clique);
        }

        /* Verifica se o jogador chegou no final da tela apos derrotar o boss */
        if (fase_terminando && jogador->px > cenario.largura - 50) {
            venceu = 1;
            break;
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
            jogador_processar_tiro(jogador, &tiros_jogador, fire_clique, fire_forte_clique,
                                   centro_mundo_x, centro_mundo_y, dx, dy);
            
            /* Registra os comandos do jogador para o copycat imitar depois */
            copycat_registrar_estado_jogador(&copycat, jogador, fire_clique, fire_forte_clique, dx, dy);
        }
        tiros_atualizar(&tiros_jogador);

        /* Atualiza Boss */
        copycat_atualizar(&copycat);
        if (!copycat.morto && copycat.history_count == COPYCAT_DELAY) {
            /* Se o jogador atirou no passado, o boss atira agora */
            int idx = copycat.history_index;
            copycat_state_t st = copycat.history[idx];
            
            /* Calcula o centro do boss para a origem do tiro */
            int b_cx = copycat.px + (copycat.frame_atual ? copycat.frame_atual->width / 2 : 0);
            int b_cy = copycat.py + (copycat.frame_atual ? copycat.frame_atual->height / 2 : 0);

            if (st.fire_clique || st.fire_forte_clique) {
                const tipo_tiro_t *tipo = st.fire_forte_clique ? &jogador->tiro_carregado : &jogador->tiro_simples;
                tiros_disparar(&tiros_boss, b_cx, b_cy, st.mira_dx, st.mira_dy, tipo);
            }
        }
        tiros_atualizar(&tiros_boss);

        /* Colisoes*/
        /* Tiros do jogador no Boss */
        if (!copycat.morto) {
            int dano_boss = tiros_colidir_com(&tiros_jogador, copycat_hitbox(&copycat));
            int dano_real = dano_boss * (jogador->buff_instakill > 0 ? 3 : 1);
            copycat_receber_dano(&copycat, dano_real);
        }

        /* Tiros do Boss no Jogador */
        int dano_hero = tiros_colidir_com(&tiros_boss, jogador_hitbox(jogador));
        jogador_receber_dano(jogador, dano_hero);

        if (copycat.morto) {
            fase_terminando = 1;
            /* O jogador pode passar pela borda direita da tela apos a morte do boss */
        }

        /* Atualiza hardware e desenha tela */
        int inimigos_restantes = copycat.morto ? 0 : 1;
        hw_display_inimigos_restantes = inimigos_restantes;
        hw_leds_vida_personagem = jogador->vida;

        desenhar_cena_fase_1_5(&cenario, jogador, &tiros_jogador, &tiros_boss, &copycat,
                               mira_tela_x, mira_tela_y, dx, dy,
                               inimigos_restantes, frame_contador, fase_terminando);

        if (jogador->estado == ESTADO_MORRER && animacao_terminou(&jogador->anim_morrer)) {
            venceu = 0;
            break;
        }
    }

    return venceu;
}

