#ifndef FASES_H
#define FASES_H

#include "personagem/jogador.h"

/* Retorna 1 se o jogador passou da fase, 0 se o jogador morreu ou saiu. */
int rodar_fase_1(jogador_t *jogador);
int rodar_fase_2(jogador_t *jogador);
int rodar_fase_3(jogador_t *jogador);

#endif
