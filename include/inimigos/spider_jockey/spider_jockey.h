#ifndef SPIDER_JOCKEY_H
#define SPIDER_JOCKEY_H

#include "sprite/sprite.h"
#include "animacao/animacao.h"
#include "colisao/colisao.h"
#include "cenario/cenario.h"
#include "framebuffer/framebuffer.h"

/* ================================================================
 * Spider Jockey — Inimigo da Fase 2
 *
 * Comportamento:
 *  - Mais rapido e com maior alcance de patrulha que o soldado.
 *  - Dispara flechas em trajetoria parabolica calculada para acertar
 *    a posicao atual do alvo no momento do disparo.
 *  - Animacao de morte: sprite invertido (de cabeca para baixo) com
 *    tint vermelho, sem explosao separada.
 * ================================================================ */

/* Constantes de movimento — ajuste fino aqui sem mexer na logica */
#define SJ_VELOCIDADE          3      /* px/frame de caminhada (3x soldado) */
#define SJ_ALCANCE_PATRULHA    600    /* distancia horizontal maxima para seguir */
#define SJ_ALCANCE_TIRO        400    /* distancia maxima para atirar (px) */
#define SJ_VIDA                8      /* pontos de vida */
#define SJ_INTERVALO_TIRO      200    /* frames entre disparos (~2s a 60fps) */

/* Fisica da flecha parabolica (mesmas unidades do engine: px e frames)
 * Com T=25 e g=0.25, o angulo de lancamento e ~21 graus para alvo a 200px
 * na mesma altura. Mais longe = angulo menor (mais raso); mais perto = mais alto.
 * Formula: angulo = atan2(0.5*g*T, dx/T) */
#define SJ_FLECHA_GRAVIDADE    0.027777f /* aceleracao gravitacional por frame^2 (~1/9 original) */
#define SJ_FLECHA_TEMPO_VOO    75     /* frames alvo para o tempo de voo da flecha (3x mais longo/lento) */
#define SJ_FLECHA_VIDA         240    /* frames antes da flecha sumir se nao acertar (3x mais longo) */

/* Dano da flecha: proporcional a velocidade relativa (mecanica Minecraft) */
#define SJ_FLECHA_DANO_BASE    1      /* dano minimo */
#define SJ_FLECHA_DANO_MAX     3      /* dano maximo (se o heroi correr de encontro) */
#define SJ_FLECHA_DANO_ESCALA  80.0f  /* divisor de px/frame para normalizar o dano */

/* Frames de pose de ataque visiveis apos disparo */
#define SJ_POSE_ATIRAR_FRAMES  18
/* Frames de flash vermelho ao tomar dano */
#define SJ_FLASH_FRAMES        8
/* Frames da animacao de morte (invertida) */
#define SJ_MORTE_FRAMES        40

typedef enum {
    SJ_ESTADO_CAINDO,      /* entrando pelo paraquedas (igual soldado) */
    SJ_ESTADO_CHAO,        /* patrulhando / atirando */
    SJ_ESTADO_PULANDO,     /* saltando para plataformas */
    SJ_ESTADO_MORRENDO     /* animacao de morte — sprite invertido + tint */
} sj_estado_t;

/* Pool de flechas parabolicas: cada flecha tem sua propria velocidade vertical
   porque a parabolica e calculada individualmente no momento do disparo. */
#define SJ_FLECHAS_MAX 8

typedef struct {
    float x, y;
    float vx, vy;      /* velocidades em px/frame; vy aumenta com gravidade */
    int   ativo;
    int   vida;        /* frames restantes antes de sumir */
    int   espelhada;   /* 1 se a flecha vai para a esquerda */
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
    animacao_t anim_morrer;     /* mesmos frames de idle — desenhados invertidos */
    animacao_t anim_paraquedas;
    int atirando_pose_frames;
    int morte_timer;            /* contador para durar a animacao de morte */

    int vida;
    int flash_frames;
    sj_estado_t estado;

    int tempo_decisao;
    int comportamento; /* 0 = seguir, 1 = parado, 2 = afastar */
    int cooldown_tiro;

    /* Sprite da flecha (ponteiro para o frame unico convertido) */
    const sprite_frame_t *sprite_flecha;

    /* Pool de flechas parabolicas propria deste inimigo */
    sj_flecha_t flechas[SJ_FLECHAS_MAX];
} spider_jockey_t;

/* Inicializa o spider jockey.
   - idle/andar/atirar/paraquedas: frames da animacao (ja convertidos para RGB565)
   - sprite_flecha: frame unico da flecha (spider_jockey_arrow_CNV)
   - chao_y: coordenada Y do chao onde ele vai pousar */
void sj_iniciar(spider_jockey_t *sj,
                int px, int py, int chao_y,
                const sprite_frame_t *idle_frames,    int idle_count,
                const sprite_frame_t *andar_frames,   int andar_count,
                const sprite_frame_t *atirar_frames,  int atirar_count,
                const sprite_frame_t *paraquedas_frames, int paraquedas_count,
                const sprite_frame_t *sprite_flecha);

/* Atualiza posicao, IA, flechas parabolicas. Coordenadas em espaco de MUNDO.
   alvo_vx / alvo_vy: velocidade do heroi neste frame (para calculo de dano). */
void sj_atualizar(spider_jockey_t *sj,
                  int alvo_x, int alvo_y,
                  float alvo_vx, float alvo_vy,
                  const cenario_t *cenario);

/* Desenha o spider jockey e suas flechas deslocados pela camera. */
void sj_desenhar(const spider_jockey_t *sj, int camera_x, int camera_y);

/* Retangulo de colisao do corpo do spider jockey (espaco de mundo). */
retangulo_t sj_hitbox(const spider_jockey_t *sj);

/* Verifica colisao das flechas com um retangulo alvo.
   Retorna o dano total causado (0 se nenhuma flecha acertou).
   Leva em conta a mecanica de dano proporcional a velocidade. */
int sj_flechas_colidir(spider_jockey_t *sj,
                       retangulo_t alvo,
                       float alvo_vx, float alvo_vy);

/* Aplica dano; inicia animacao de morte se vida <= 0. */
void sj_receber_dano(spider_jockey_t *sj, int dano);

/* 1 quando a animacao de morte terminou (pode ser removido do jogo). */
int sj_esta_morto(const spider_jockey_t *sj);

#endif /* SPIDER_JOCKEY_H */
