#include "cenario/fases.h"
#include "cenario/fase3/fase3.h"
#include "personagem/jogador.h"
#include "inimigo/inimigo.h"
#include "inimigos/soldado/soldado.h"
#include "inimigos/tank/tank.h"
#include "inimigos/goomba/goomba.h"
#include "ui/ui.h"
#include "hardware/hardware_state.h"
#include <stdlib.h>
#include <math.h>

#define FASE3_TOTAL_GOOMBAS  15
#define FASE3_TOTAL_SOLDADOS 15
#define FASE3_TOTAL_INIMIGOS (FASE3_TOTAL_GOOMBAS + FASE3_TOTAL_SOLDADOS)

#define GOOMBA_SIMULTANEOS   5
#define SOLDADOS_SIMULTANEOS 3

typedef struct {
    inimigo_t inimigo;
    int ativo;
} soldado_slot_t;

static void desenhar_cena_fase_3(const cenario_t *c, const jogador_t *j,
                                 const tiros_t *tiros_jogador, const tank_t *tank,
                                 const goomba_t *goombas, const soldado_slot_t *soldados,
                                 const tiros_t *tiros_inimigos,
                                 int frame_contador, float dx, float dy,
                                 int mira_tela_x, int mira_tela_y, int inimigos_restantes, int fase_terminando)
{
    fb_clear(fb_rgb(0, 0, 0));
    cenario_desenhar_bg(c);

    tank_desenhar(tank, c->camera_x, c->camera_y);

    jogador_desenhar(j, c->camera_x, c->camera_y);
    tiros_desenhar(tiros_jogador, c->camera_x, c->camera_y);

    for (int i = 0; i < GOOMBA_SIMULTANEOS; i++) {
        goomba_desenhar(&goombas[i], c->camera_x, c->camera_y);
    }

    for (int i = 0; i < SOLDADOS_SIMULTANEOS; i++) {
        if (soldados[i].ativo) {
            inimigo_desenhar(&soldados[i].inimigo, c->camera_x, c->camera_y);
        }
    }
    
    tiros_desenhar(tiros_inimigos, c->camera_x, c->camera_y);

    cenario_desenhar_fg(c);

    desenhar_icones(c->camera_x, c->camera_y, frame_contador);
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

int rodar_fase_3(jogador_t *jogador) {
    cenario_t cenario;
    cenario_iniciar(&cenario, fase3_bg, NULL, fase3_colisao, FASE3_LARGURA, FASE3_ALTURA);

    tiros_t tiros;
    tiros_iniciar(&tiros);
    
    tiros_t tiros_inimigos;
    tiros_iniciar(&tiros_inimigos);

    tank_t tank;
    tank_iniciar(&tank, cenario.largura - 80, cenario.altura - 34);

    goomba_t goombas[GOOMBA_SIMULTANEOS];
    for (int i = 0; i < GOOMBA_SIMULTANEOS; i++) {
        goombas[i].estado = GOOMBA_INATIVO;
    }

    soldado_slot_t soldados[SOLDADOS_SIMULTANEOS];
    for (int i = 0; i < SOLDADOS_SIMULTANEOS; i++) {
        soldados[i].ativo = 0;
    }

    int goombas_mortos = 0;
    int soldados_spawnados = 0;
    int soldados_mortos = 0;
    
    int inimigos_restantes = FASE3_TOTAL_INIMIGOS;
    hw_display_inimigos_restantes = inimigos_restantes;

    int frame_contador = 0;
    int debug_pass_last = fb_key_down(FB_KEY_DEBUG_PASS);
    int venceu = 0;
    int fase_terminando = 0;
    int fase_entrada = 1;
    
    tipo_tiro_t tiro_soldado = { &soldado_tiro_frames[0], 3, 1 };

    jogador->px = 20; 

    while (!fb_poll_quit()) {
        if (hw_jogo_pausado) {
            desenhar_cena_fase_3(&cenario, jogador, &tiros, &tank, goombas, soldados, &tiros_inimigos, frame_contador, 0, 0, 0, 0, FASE3_TOTAL_INIMIGOS - (goombas_mortos + soldados_mortos), fase_terminando);
            fb_present();
            continue;
        }

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

        hw_leds_vida_personagem = jogador->vida;
        hw_display_inimigos_restantes = inimigos_restantes;

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

        if (jogador->estado != ESTADO_MORRER && !fase_terminando) {
            if (dx < -0.01f) jogador->direcao = -1;
            else if (dx > 0.01f) jogador->direcao = 1;
        }
        if (fase_terminando) jogador->direcao = 1;

        jogador_atualizar_animacao(jogador, dy);

        if (!fase_entrada) {
            jogador_processar_tiro(jogador, &tiros, fire_clique, fire_forte_clique,
                                   centro_mundo_x, centro_mundo_y, dx, dy);
        }
        tiros_atualizar(&tiros);
        tiros_atualizar(&tiros_inimigos);

        if (!fase_terminando && (goombas_mortos >= FASE3_TOTAL_GOOMBAS && soldados_mortos >= FASE3_TOTAL_SOLDADOS)) {
            fase_terminando = 1;
            tank.estado = TANK_DEATH;
        }
        
        if (fase_terminando && jogador->px > cenario.largura - JOGADOR_LARGURA - 10) {
            venceu = 1;
            break;
        }

        if (jogador->estado == ESTADO_MORRER && animacao_terminou(&jogador->anim_morrer)) {
            venceu = 0;
            break;
        }

        if (!fase_terminando) {
            tank_atualizar(&tank, goombas, GOOMBA_SIMULTANEOS, jogador->px);
        }

        for (int i = 0; i < GOOMBA_SIMULTANEOS; i++) {
            if (goombas[i].estado == GOOMBA_INATIVO) continue;
            
            int esmagado_antes = (goombas[i].estado == GOOMBA_ESMAGADO);
            float old_vy = goombas[i].vy;
            
            goomba_atualizar(&goombas[i], &cenario);
            
            if (goombas[i].estado == GOOMBA_VOANDO && old_vy < 0.0f && goombas[i].vy >= 0.0f) {
                goombas[i].x = (float)jogador->px;
                goombas[i].vx = 0.0f;
            }
            
            if (goombas[i].estado == GOOMBA_ANDANDO || goombas[i].estado == GOOMBA_VOANDO || goombas[i].estado == GOOMBA_PULANDO) {
                retangulo_t h_g = goomba_hitbox(&goombas[i]);
                retangulo_t h_j = jogador_hitbox(jogador);
                
                if (colisao_retangulos(h_j, h_g)) {
                    if (jogador->vel_y > 0 && h_j.y + h_j.altura - 10 < h_g.y + h_g.altura/2 && (goombas[i].estado == GOOMBA_ANDANDO || goombas[i].estado == GOOMBA_PULANDO)) {
                        goomba_esmagar(&goombas[i]);
                        goombas_mortos++;
                        inimigos_restantes = FASE3_TOTAL_INIMIGOS - (goombas_mortos + soldados_mortos);
                    } else {
                        if (goombas[i].estado == GOOMBA_VOANDO) {
                            jogador_receber_dano_knockback(jogador, 1, goombas[i].x);
                        } else if (goombas[i].estado == GOOMBA_ANDANDO) {
                            jogador_receber_dano_knockback(jogador, 1, goombas[i].x);
                            goombas[i].direcao = -goombas[i].direcao;
                        } else if (goombas[i].estado == GOOMBA_PULANDO) {
                            jogador_receber_dano_knockback(jogador, 1, goombas[i].x);
                            goombas[i].direcao = -goombas[i].direcao;
                        }
                    }
                }
            }
        }

        if (!fase_terminando && soldados_spawnados < FASE3_TOTAL_SOLDADOS && rand() % 200 == 0) {
            for (int i = 0; i < SOLDADOS_SIMULTANEOS; i++) {
                if (!soldados[i].ativo) {
                    int s_x = 20 + rand() % (cenario.largura - 100);
                    inimigo_iniciar(&soldados[i].inimigo, s_x, cenario.camera_y - 80, cenario_chao_y(&cenario, s_x + 38/2, 0) - 45, 4,
                                     soldado_parado_frames, SOLDADO_PARADO_FRAME_COUNT, 10,
                                     soldado_correr_frames, SOLDADO_CORRER_FRAME_COUNT, 5,
                                     soldado_atirar_frames, SOLDADO_ATIRAR_FRAME_COUNT, 4,
                                     soldado_morrer_frames, SOLDADO_MORRER_FRAME_COUNT, 6,
                                     soldado_explosao_frames, SOLDADO_EXPLOSAO_FRAME_COUNT, 4,
                                     soldado_paraquedas_frames, SOLDADO_PARAQUEDAS_FRAME_COUNT, 8,
                                     tiro_soldado, 180);
                    soldados[i].ativo = 1;
                    soldados_spawnados++;
                    break;
                }
            }
        }

        for (int i = 0; i < SOLDADOS_SIMULTANEOS; i++) {
            if (!soldados[i].ativo) continue;
            
            inimigo_atualizar(&soldados[i].inimigo, &tiros_inimigos, centro_mundo_x, centro_mundo_y, &cenario);
            
            retangulo_t hs = inimigo_hitbox(&soldados[i].inimigo);
            int acertou = tiros_colidir_com(&tiros, hs);
            if (acertou > 0) {
                inimigo_receber_dano(&soldados[i].inimigo, acertou * (jogador->buff_instakill > 0 ? 3 : 1));
            }
            
            if (inimigo_esta_morto(&soldados[i].inimigo)) {
                if (rand() % 100 < 30) {
                    for (int k = 0; k < MAX_ICONES; k++) {
                        if (!icones_mapa[k].ativo) {
                            icones_mapa[k].ativo = 1;
                            int r_tipo = rand() % 100;
                            if      (r_tipo < 50) icones_mapa[k].tipo = ICONE_VIDA;
                            else if (r_tipo < 67) icones_mapa[k].tipo = ICONE_VELOCIDADE;
                            else if (r_tipo < 84) icones_mapa[k].tipo = ICONE_INSTAKILL;
                            else                  icones_mapa[k].tipo = ICONE_SUPER_PULO;
                            retangulo_t h = inimigo_hitbox(&soldados[i].inimigo);
                            icones_mapa[k].px = h.x + h.largura  / 2 - 8;
                            icones_mapa[k].py = h.y + h.altura - 16;
                            break;
                        }
                    }
                }
                soldados[i].ativo = 0;
                soldados_mortos++;
                inimigos_restantes = FASE3_TOTAL_INIMIGOS - (goombas_mortos + soldados_mortos);
            }
        }
        
        int acertos_no_jogador = tiros_colidir_com(&tiros_inimigos, jogador_hitbox(jogador));
        jogador_receber_dano(jogador, acertos_no_jogador);

        if (inimigos_restantes < 0) inimigos_restantes = 0;

        desenhar_cena_fase_3(&cenario, jogador, &tiros, &tank, goombas, soldados, &tiros_inimigos,
                             frame_contador, dx, dy, mira_tela_x, mira_tela_y, inimigos_restantes, fase_terminando);
    }
    return venceu;
}
