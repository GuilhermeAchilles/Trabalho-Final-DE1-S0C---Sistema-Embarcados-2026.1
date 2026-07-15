/* Inimigo boss Spider Jockey */

#ifndef SPIDER_JOCKEY_H
#define SPIDER_JOCKEY_H

#include "sprite/sprite.h"
#include "animacao/animacao.h"
#include "colisao/colisao.h"
#include "cenario/cenario.h"
#include "framebuffer/framebuffer.h"

/* Spider Jockey - inimigo da fase 2.
   Anda mais rapido que o soldado e atira flechas em arco (parabolicas). */

/* Parametros de movimento */
#define SJ_VELOCIDADE          3      /* velocidade de caminhada em px/frame */
#define SJ_ALCANCE_PATRULHA    600    /* distancia max que ele segue o jogador */
#define SJ_ALCANCE_TIRO        400    /* distancia max pra ele atirar */
#define SJ_VIDA                8
#define SJ_INTERVALO_TIRO      200    /* frames entre cada tiro */

/* Parametros da flecha */
#define SJ_FLECHA_GRAVIDADE    0.027777f /* gravidade aplicada na flecha por frame */
#define SJ_FLECHA_TEMPO_VOO    75     /* tempo estimado de voo em frames */
#define SJ_FLECHA_VIDA         240    /* frames ate a flecha sumir sozinha */

/* Dano da flecha */
#define SJ_FLECHA_DANO_BASE    1
#define SJ_FLECHA_DANO_MAX     3
#define SJ_FLECHA_DANO_ESCALA  80.0f  /* fator de normalizacao do dano */

/* Timers de animacao */
#define SJ_POSE_ATIRAR_FRAMES  18
#define SJ_FLASH_FRAMES        8
#define SJ_MORTE_FRAMES        40

typedef enum {
    SJ_ESTADO_CAINDO,   /* caindo de paraquedas */
    SJ_ESTADO_CHAO,     /* andando e atirando */
    SJ_ESTADO_PULANDO,
    SJ_ESTADO_MORRENDO
} sj_estado_t;

/* Cada flecha tem velocidade propria porque o arco e calculado na hora do disparo */
#define SJ_FLECHAS_MAX 8

typedef struct {
    float x, y;
    float vx, vy;      /* velocidade em px/frame */
    int   ativo;
    int   vida;        /* frames restantes */
    int   espelhada;   /* 1 = indo pra esquerda */
    const sprite_frame_t *sprite;
} sj_flecha_t;

typedef struct {
    int px, py;
    float vel_y;
    int chao_y;
    int direcao;       /* 1 = direita, -1 = esquerda */
    const sprite_frame_t *frame;

    animacao_t anim_idle;
    animacao_t anim_andar;
    animacao_t anim_atirar;
    animacao_t anim_morrer;
    animacao_t anim_paraquedas;
    int atirando_pose_frames;
    int morte_timer;

    int vida;
    int flash_frames;
    sj_estado_t estado;

    int tempo_decisao;
    int comportamento; /* 0 = seguir, 1 = parado, 2 = afastar */
    int cooldown_tiro;

    const sprite_frame_t *sprite_flecha;
    sj_flecha_t flechas[SJ_FLECHAS_MAX];
} spider_jockey_t;

/* Inicializa o spider jockey na posicao (px, py) com chao em chao_y */
void sj_iniciar(spider_jockey_t *sj,
                int px, int py, int chao_y,
                const sprite_frame_t *idle_frames,    int idle_count,
                const sprite_frame_t *andar_frames,   int andar_count,
                const sprite_frame_t *atirar_frames,  int atirar_count,
                const sprite_frame_t *paraquedas_frames, int paraquedas_count,
                const sprite_frame_t *sprite_flecha);

/* Atualiza IA, posicao e flechas. Recebe posicao e velocidade do jogador */
void sj_atualizar(spider_jockey_t *sj,
                  int alvo_x, int alvo_y,
                  float alvo_vx, float alvo_vy,
                  const cenario_t *cenario);

/* Desenha o spider jockey e suas flechas na tela */
void sj_desenhar(const spider_jockey_t *sj, int camera_x, int camera_y);

/* Retorna a hitbox do spider jockey */
retangulo_t sj_hitbox(const spider_jockey_t *sj);

/* Verifica se alguma flecha acertou o alvo. Retorna o dano total */
int sj_flechas_colidir(spider_jockey_t *sj,
                       retangulo_t alvo,
                       float alvo_vx, float alvo_vy);

/* Aplica dano no spider jockey */
void sj_receber_dano(spider_jockey_t *sj, int dano);

/* Retorna 1 se o spider jockey ja morreu e pode ser removido */
int sj_esta_morto(const spider_jockey_t *sj);

#endif
