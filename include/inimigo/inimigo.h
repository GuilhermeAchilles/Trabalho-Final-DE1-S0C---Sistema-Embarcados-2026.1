/* Estruturas base e gerenciador de inimigos */

#ifndef INIMIGO_H
#define INIMIGO_H

#include "sprite/sprite.h"
#include "animacao/animacao.h"
#include "ataques/tiros/tiro.h"
#include "colisao/colisao.h"
#include "cenario/cenario.h"

/* Inimigo generico (soldado). A aparencia e o tipo de tiro
   sao definidos nos parametros de inimigo_iniciar. */
typedef enum {
    INIMIGO_ESTADO_CAINDO,
    INIMIGO_ESTADO_CHAO,
    INIMIGO_ESTADO_PULANDO,
    INIMIGO_ESTADO_MORRENDO,
    INIMIGO_ESTADO_EXPLODINDO
} inimigo_estado_t;

typedef struct {
    int px, py;
    float vel_y;
    int chao_y;
    int direcao; /* 1 = direita, -1 = esquerda */
    const sprite_frame_t *frame;

    animacao_t anim_idle;
    animacao_t anim_andar;
    animacao_t anim_atirar;
    animacao_t anim_morrer;
    animacao_t anim_explosao;
    animacao_t anim_paraquedas;
    int atirando_pose_frames;

    tipo_tiro_t tiro;
    int intervalo_tiro;  /* frames entre cada disparo */
    int cooldown_tiro;

    int vida;
    int flash_frames;
    inimigo_estado_t estado;

    int tempo_decisao;
    int comportamento; /* 0 = seguir, 1 = parado, 2 = fugir */
} inimigo_t;

/* Inicializa o inimigo com posicao, vida e todas as animacoes */
void inimigo_iniciar(inimigo_t *inimigo, int px, int py, int chao_y, int vida,
                      const sprite_frame_t *idle_frames, int idle_frame_count, int idle_frames_por_sprite,
                      const sprite_frame_t *andar_frames, int andar_frame_count, int andar_frames_por_sprite,
                      const sprite_frame_t *atirar_frames, int atirar_frame_count, int atirar_frames_por_sprite,
                      const sprite_frame_t *morrer_frames, int morrer_frame_count, int morrer_frames_por_sprite,
                      const sprite_frame_t *explosao_frames, int explosao_frame_count, int explosao_frames_por_sprite,
                      const sprite_frame_t *paraquedas_frames, int paraquedas_frame_count, int paraquedas_frames_por_sprite,
                      tipo_tiro_t tiro, int intervalo_tiro);

/* Atualiza a IA, movimentacao e disparo do inimigo */
void inimigo_atualizar(inimigo_t *inimigo, tiros_t *tiros, int alvo_x, int alvo_y, const cenario_t *cenario);

/* Desenha o inimigo na tela (pisca vermelho ao tomar dano) */
void inimigo_desenhar(const inimigo_t *inimigo, int camera_x, int camera_y);

/* Retorna a hitbox do inimigo */
retangulo_t inimigo_hitbox(const inimigo_t *inimigo);

/* Aplica dano no inimigo e ativa o flash vermelho */
void inimigo_receber_dano(inimigo_t *inimigo, int dano);

/* Retorna 1 se o inimigo morreu e a animacao de morte acabou */
int inimigo_esta_morto(const inimigo_t *inimigo);

#endif
