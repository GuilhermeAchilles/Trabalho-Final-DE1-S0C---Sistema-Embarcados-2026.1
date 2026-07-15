/* Assinaturas das funcoes que rodam cada fase do jogo */

#ifndef FASES_H
#define FASES_H

#include "personagem/jogador.h"

/* Cada funcao roda uma fase do jogo.
   Retorna 1 se o jogador passou de fase, 0 se morreu ou saiu. */
int rodar_fase_1(jogador_t *jogador);
int rodar_fase_2(jogador_t *jogador);
int rodar_fase_3(jogador_t *jogador);

#endif
