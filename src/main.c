/* Utilidade: Ponto de entrada (Main): coordena o salto entre menus e fases */
#include <stdlib.h>
#include <stdio.h>

#include "framebuffer/framebuffer.h"
#include "personagem/jogador.h"
#include "ui/ui.h"
#include "cenario/fases.h"
#include "cenario/fase1_5/fase1_5.h"

#include "menu/menu.h"
#include "game_over/game_over.h"
#include "final_tela/final_tela.h"

/* Estados da maquina de estados principal do jogo */
typedef enum {
    ESTADO_MENU,
    ESTADO_FASE1,
    ESTADO_FASE1_5,
    ESTADO_FASE2,
    ESTADO_FASE3,
    ESTADO_FINAL,
    ESTADO_GAME_OVER,
    ESTADO_SAIR
} estado_jogo_t;

/* Funcao principal: inicializa o jogo e gerencia as transicoes de tela/fase */
int main(int argc, char *argv[]) {
    /* Inicializa o hardware de video (framebuffer) */
    fb_init();

    /* O jogo sempre comeca na tela de menu */
    estado_jogo_t estado_atual = ESTADO_MENU;

    /* Loop infinito ate o jogador fechar o jogo */
    while (estado_atual != ESTADO_SAIR) {
        if (estado_atual == ESTADO_MENU) {
            /* Retorna 1 se escolheu jogar, 0 se escolheu sair */
            int iniciar = tela_menu();
            if (iniciar) {
                estado_atual = ESTADO_FASE1;
            } else {
                estado_atual = ESTADO_SAIR;
            }
        } else if (estado_atual == ESTADO_FASE1) {
            /* Cria e posiciona o jogador na primeira fase */
            jogador_t jogador;
            jogador_iniciar(&jogador, -30, 20);

            /* Chama a logica da Fase 1, que trava o loop ate alguem ganhar ou perder */
            int passou = rodar_fase_1(&jogador);

            if (passou == 1) {
                printf("Parabens! Voce sobreviveu a Fase 1 e matou 10 soldados!\n");
                estado_atual = ESTADO_FASE2; 
            } else if (passou == 2) {
                printf("Easter Egg Encontrado! Entrando na Fase 1.5!\n");
                estado_atual = ESTADO_FASE1_5;
            } else {
                printf("Game Over! Voce morreu na Fase 1.\n");
                estado_atual = ESTADO_GAME_OVER;
            }
        } else if (estado_atual == ESTADO_FASE1_5) {
            /* Fase secreta (Copycat). O jogador spawna em outra posicao */
            jogador_t jogador;
            jogador_iniciar(&jogador, -30, 160);

            int passou = rodar_fase_1_5(&jogador);

            if (passou) {
                printf("Parabens! Voce venceu a Fase 1.5 e o Boss Copycat! Pulando para o final!\n");
                estado_atual = ESTADO_FINAL; 
            } else {
                printf("Game Over! Voce morreu na Fase 1.5.\n");
                estado_atual = ESTADO_GAME_OVER;
            }
        } else if (estado_atual == ESTADO_FASE2) {
            jogador_t jogador;
            jogador_iniciar(&jogador, -30, 160);

            int passou = rodar_fase_2(&jogador);

            if (passou) {
                printf("Parabens! Voce venceu a Fase 2!\n");
                estado_atual = ESTADO_FASE3; 
            } else {
                printf("Game Over! Voce morreu na Fase 2.\n");
                estado_atual = ESTADO_GAME_OVER;
            }
        } else if (estado_atual == ESTADO_FASE3) {
            jogador_t jogador;
            jogador_iniciar(&jogador, -30, 160);

            int passou = rodar_fase_3(&jogador);

            if (passou) {
                printf("Parabens! Voce fechou o jogo e passou da Fase 3!\n");
                estado_atual = ESTADO_FINAL; 
            } else {
                printf("Game Over! Voce morreu na Fase 3.\n");
                estado_atual = ESTADO_GAME_OVER;
            }
        } else if (estado_atual == ESTADO_GAME_OVER) {
            int reiniciar = tela_game_over_nova();
            if (reiniciar) {
                estado_atual = ESTADO_FASE1;
            } else {
                estado_atual = ESTADO_SAIR;
            }
        } else if (estado_atual == ESTADO_FINAL) {
            int iniciar = tela_final();
            if (iniciar) {
                estado_atual = ESTADO_MENU;
            } else {
                estado_atual = ESTADO_SAIR;
            }
        }
    }

    /* Encerra a comunicacao de video com seguranca */
    fb_shutdown();
    return 0;
}
