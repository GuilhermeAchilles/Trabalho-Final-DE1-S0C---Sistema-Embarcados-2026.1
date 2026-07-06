#ifndef ACHILLES_TIRO_H
#define ACHILLES_TIRO_H

#include <stdint.h>
#include "sprite/sprite.h"

/* Achilles ainda nao tem pixel art de projetil - placeholder amarelo 4x2 ate ter arte de verdade. */
static const uint16_t achilles_tiro_data[4 * 2] = {
    0xFFE0, 0xFFE0, 0xFFE0, 0xFFE0,
    0xFFE0, 0xFFE0, 0xFFE0, 0xFFE0,
};

static const sprite_frame_t achilles_tiro_frames[] = {
    { 4, 2, achilles_tiro_data },
};
#define ACHILLES_TIRO_FRAME_COUNT 1

#endif /* ACHILLES_TIRO_H */
