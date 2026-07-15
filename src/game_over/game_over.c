/* Utilidade: Logica da tela exibida quando o jogador morre (derrota) */
#include "game_over/game_over.h"
#include "framebuffer/framebuffer.h"
#include "menu/menu.h"

/* Loop da tela de Game Over. Mostra a imagem de derrota e aguarda o START pra reiniciar */
int tela_game_over_nova(void) {
    int reiniciar = 0;
    int frame_counter = 0;
    while (!fb_poll_quit()) {
        for (int y = 0; y < GAME_OVER_HEIGHT; y++) {
            for (int x = 0; x < GAME_OVER_WIDTH; x++) {
                fb_put_pixel(x, y, game_over_bg[y * GAME_OVER_WIDTH + x]);
            }
        }
        
        /* Logica de piscar (Blink): mostra o texto por 30 frames e esconde por 30 frames */
        if ((frame_counter / 30) % 2 == 0) {
            int start_x = (GAME_OVER_WIDTH - PRESS_START_WIDTH) / 2;
            int start_y = GAME_OVER_HEIGHT - PRESS_START_HEIGHT - 20; /* Margem de 20 pixels da borda inferior */
            for (int y = 0; y < PRESS_START_HEIGHT; y++) {
                for (int x = 0; x < PRESS_START_WIDTH; x++) {
                    uint16_t color = press_start_data[y * PRESS_START_WIDTH + x];
                    if (color != PRESS_START_CHROMA_KEY) {
                        fb_put_pixel(start_x + x, start_y + y, color);
                    }
                }
            }
        }
        frame_counter++;

        fb_present();
        
        if (frame_counter > 30) {
            if (fb_key_down(FB_KEY_FIRE)) {
                reiniciar = 1;
                break;
            }
            if (fb_key_down(FB_KEY_FIRE_FORTE)) {
                reiniciar = 0;
                break;
            }
        }
    }
    return reiniciar;
}
