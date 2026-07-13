#ifndef FASE1_5_H
#define FASE1_5_H

#include <stdint.h>
#include "personagem/jogador.h"

#define FASE1_5_LARGURA 320
#define FASE1_5_ALTURA 240
#define FASE1_5_PIXELS (FASE1_5_LARGURA * FASE1_5_ALTURA)

extern const uint16_t fase1_5_bg[FASE1_5_PIXELS];
extern const uint8_t fase1_5_colisao[FASE1_5_PIXELS];

int rodar_fase_1_5(jogador_t *jogador);

#endif
