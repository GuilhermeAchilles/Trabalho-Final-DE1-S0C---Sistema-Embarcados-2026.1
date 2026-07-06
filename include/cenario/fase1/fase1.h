#ifndef FASE1_H
#define FASE1_H

#include <stdint.h>

/* Dados de pixel da fase 1 (RGB565), portados do cenario feito pelo Marcus.
   Tres camadas alinhadas pixel-a-pixel num mundo FASE1_LARGURA x FASE1_ALTURA:
     - fase1_bg      = fundo (sempre desenhado)
     - fase1_fg      = frente do terreno (0x0000 = transparente, sobrepoe o fundo)
     - fase1_colisao = mapa de colisao, 1 byte por pixel
                       (0 vazio, 1 solido, 2 plataforma, 3 lava) */

#define FASE1_LARGURA 480
#define FASE1_ALTURA  360
#define FASE1_PIXELS  (FASE1_LARGURA * FASE1_ALTURA)

extern const uint16_t fase1_bg[FASE1_PIXELS];
extern const uint16_t fase1_fg[FASE1_PIXELS];
extern const uint8_t  fase1_colisao[FASE1_PIXELS];

#endif /* FASE1_H */
