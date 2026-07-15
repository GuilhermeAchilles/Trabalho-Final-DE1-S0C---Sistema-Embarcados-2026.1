/* Tela de Game Over */

#ifndef GAME_OVER_H
#define GAME_OVER_H

#include <stdint.h>

#define GAME_OVER_WIDTH  320
#define GAME_OVER_HEIGHT 240
#define GAME_OVER_PIXELS (320 * 240)

extern const uint16_t game_over_bg[GAME_OVER_PIXELS];

/* Retorna 1 para reiniciar o jogo, 0 para sair */
int tela_game_over_nova(void);

#endif /* GAME_OVER_H */
