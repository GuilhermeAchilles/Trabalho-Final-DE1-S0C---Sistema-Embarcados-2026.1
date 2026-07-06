#include <stdlib.h>
#include <stdio.h>

#include "framebuffer/framebuffer.h"
#include "personagem/jogador.h"
#include "ui/ui.h"
#include "cenario/fases.h"

/* Retorna 1 se o jogador quer reiniciar, 0 se quer sair */
int tela_game_over(void) {
    int restart = 0;
    while (!fb_poll_quit()) {
        fb_clear(fb_rgb(0, 0, 0));
        
        desenhar_texto("GAME OVER", FB_WIDTH / 2 - 36, FB_HEIGHT / 2 - 20, 2, fb_rgb(255, 0, 0));
        desenhar_texto("CLIQUE ESQUERDO REINICIAR", FB_WIDTH / 2 - 100, FB_HEIGHT / 2 + 10, 1, fb_rgb(255, 255, 255));
        desenhar_texto("CLIQUE DIREITO SAIR", FB_WIDTH / 2 - 76, FB_HEIGHT / 2 + 25, 1, fb_rgb(255, 255, 255));
        
        fb_present();
        
        if (fb_key_down(FB_KEY_FIRE)) {
            restart = 1;
            break;
        }
        if (fb_key_down(FB_KEY_FIRE_FORTE)) {
            restart = 0;
            break;
        }
    }
    return restart;
}

int main(int argc, char *argv[]) {
    fb_init();

    while (1) {
        jogador_t jogador;
        jogador_iniciar(&jogador, 30, 20);

        int passou = rodar_fase_1(&jogador);

        if (passou) {
            printf("Parabens! Voce sobreviveu a Fase 1 e matou 50 soldados!\n");
            // rodar_fase_2(&jogador);
            break; // Sai por enquanto
        } else {
            printf("Game Over! Voce morreu na Fase 1.\n");
            int quer_reiniciar = tela_game_over();
            if (!quer_reiniciar) {
                break;
            }
        }
    }

    fb_shutdown();
    return 0;
}
