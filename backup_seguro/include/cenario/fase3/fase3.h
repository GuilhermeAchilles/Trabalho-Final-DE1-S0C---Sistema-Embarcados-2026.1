#ifndef FASE3_H
#define FASE3_H

#include <stdint.h>

#define FASE3_LARGURA  422
#define FASE3_ALTURA 240
#define FASE3_PIXELS (FASE3_LARGURA * FASE3_ALTURA)

extern const uint16_t fase3_bg[FASE3_PIXELS];
extern const uint8_t fase3_colisao[FASE3_PIXELS];

#endif
