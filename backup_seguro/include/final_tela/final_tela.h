#ifndef FINAL_TELA_H
#define FINAL_TELA_H

#include <stdint.h>

#define FINAL_TELA_WIDTH  320
#define FINAL_TELA_HEIGHT 240
#define FINAL_TELA_PIXELS (320 * 240)

extern const uint16_t final_tela_bg[FINAL_TELA_PIXELS];

/* Retorna 1 se o jogador apertou botão, para seguir para o Menu */
int tela_final(void);

#endif /* FINAL_TELA_H */
