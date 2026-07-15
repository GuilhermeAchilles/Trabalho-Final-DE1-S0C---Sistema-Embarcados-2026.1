/* Inimigo Boss Copycat */

#ifndef COPYCAT_H
#define COPYCAT_H

#include "personagem/jogador.h"

/* Delay de frames entre o jogador e o copycat (ele copia com atraso) */
#define COPYCAT_DELAY 3

/* Estado salvo de um frame do jogador (pra o copycat reproduzir depois) */
typedef struct {
    int px;
    int py;
    int direcao;
    const sprite_frame_t *frame_animacao;
    
    int fire_clique;
    int fire_forte_clique;
    float mira_dx;
    float mira_dy;
} copycat_state_t;

/* Boss Copycat - copia os movimentos do jogador com atraso */
typedef struct {
    copycat_state_t history[COPYCAT_DELAY]; /* historico de estados do jogador */
    int history_index;
    int history_count;
    
    int px;
    int py;
    int direcao;
    const sprite_frame_t *frame_atual;
    
    int vida;
    int flash_frames;
    int morto;
} copycat_t;

/* Inicializa o copycat na posicao (px, py) */
void copycat_iniciar(copycat_t *cc, int px, int py);

/* Salva o estado atual do jogador no historico do copycat */
void copycat_registrar_estado_jogador(copycat_t *cc, const jogador_t *j, int fire_clique, int fire_forte_clique, float dx, float dy);

/* Atualiza o copycat (reproduz o estado mais antigo do historico) */
void copycat_atualizar(copycat_t *cc);

/* Aplica dano no copycat */
void copycat_receber_dano(copycat_t *cc, int dano);

/* Desenha o copycat na tela */
void copycat_desenhar(const copycat_t *cc, int camera_x, int camera_y);

/* Retorna a hitbox do copycat */
retangulo_t copycat_hitbox(const copycat_t *cc);

#endif
