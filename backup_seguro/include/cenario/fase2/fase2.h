#ifndef FASE2_H
#define FASE2_H

#include <stdint.h>

#define FASE2_LARGURA  985
#define FASE2_ALTURA 240
#define FASE2_PIXELS (FASE2_LARGURA * FASE2_ALTURA)

extern const uint16_t fase2_bg[FASE2_PIXELS];
extern const uint16_t fase2_fg[FASE2_PIXELS];
extern const uint8_t fase2_colisao[FASE2_PIXELS];

#endif
