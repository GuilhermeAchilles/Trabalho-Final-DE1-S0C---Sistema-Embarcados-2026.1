/* Tela de Menu Principal */

#ifndef MENU_H
#define MENU_H

#include <stdint.h>

#define MENU_WIDTH  320
#define MENU_HEIGHT 240
#define MENU_PIXELS (320 * 240)

extern const uint16_t menu_bg[MENU_PIXELS];

#define PRESS_START_WIDTH 220
#define PRESS_START_HEIGHT 32
#define PRESS_START_CHROMA_KEY 0xF81F
extern const uint16_t press_start_data[7040];

/* Retorna 1 para iniciar o jogo, 0 para sair */
int tela_menu(void);

#endif /* MENU_H */
