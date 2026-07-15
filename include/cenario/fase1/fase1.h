/* Mapas de pixel da Fase 1 (fundo, frente e colisao) */

#ifndef FASE1_H
#define FASE1_H

#include <stdint.h>

/* Mapas de pixel da Fase 1 (fundo, frente e colisao)
   O tamanho do mapa e FASE1_LARGURA x FASE1_ALTURA */

#define FASE1_LARGURA 480
#define FASE1_ALTURA  360
#define FASE1_PIXELS  (FASE1_LARGURA * FASE1_ALTURA)

extern const uint16_t fase1_bg[FASE1_PIXELS];
extern const uint16_t fase1_fg[FASE1_PIXELS];
extern const uint8_t  fase1_colisao[FASE1_PIXELS];

#endif /* FASE1_H */
