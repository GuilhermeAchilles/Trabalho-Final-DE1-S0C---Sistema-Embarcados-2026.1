/* Controle, fisica e status do jogador (MegaMan) */

#ifndef JOGADOR_H
#define JOGADOR_H

#include "sprite/sprite.h"
#include "animacao/animacao.h"
#include "cenario/cenario.h"
#include "ataques/tiros/tiro.h"
#include "colisao/colisao.h"

#define JOGADOR_LARGURA 20
#define JOGADOR_ALTURA  46

/* Rastro visual que aparece atras do jogador com power-up ativo */
#define RASTRO_MAX 10
typedef struct {
    int px, py;
    const sprite_frame_t *frame;
    int direcao;
    int vida;
    int eh_verde;
} rastro_t;

typedef enum {
    ESTADO_IDLE,
    ESTADO_ANDAR,
    ESTADO_PULAR,
    ESTADO_ATIRAR,
    ESTADO_MORRER
} estado_t;

typedef struct {
    int px, py;
    int spawn_x, spawn_y;
    int direcao;
    estado_t estado;
    const sprite_frame_t *frame;

    float vel_y;
    int no_chao;
    int pulo_duplo_usado;

    /* Animacoes do personagem */
    animacao_t anim_idle;
    animacao_t anim_andar;
    animacao_t anim_pulo;
    animacao_t anim_morrer;
    animacao_t anim_atirar;
    animacao_t anim_atirar_cima;
    animacao_t anim_atirar_baixo;
    animacao_t anim_atirar_diag_cima;
    animacao_t anim_atirar_diag_baixo;

    /* Controle de input (pra detectar clique vs segurar) */
    int acao_pressionada_antes;
    int fire_pressionado_antes;
    int fire_forte_pressionado_antes;
    int pulo_pressionado_antes;

    /* Sistema de tiro */
    tipo_tiro_t tiro_simples;
    tipo_tiro_t tiro_carregado;
    int cooldown_tiro;
    int atirando_pose_frames;

    int tiros_carregados_restantes;  /* municao do tiro forte */
    int cooldown_recarregar_forte;
    int tiros_normais_restantes;     /* overheat do tiro normal */
    int cooldown_superaquecimento;

    /* Vida e buffs */
    int vida;
    int buff_velocidade;
    int buff_instakill;
    int buff_super_pulo;
    rastro_t rastros[RASTRO_MAX];
    int rastro_idx;

    /* Dano e knockback */
    int dano_piscar_frames;
    int timer_invulnerabilidade;
    int knockback_vx;
} jogador_t;

/* Inicializa o jogador na posicao de spawn */
void jogador_iniciar(jogador_t *j, int spawn_x, int spawn_y);

/* Retorna a hitbox do jogador */
retangulo_t jogador_hitbox(const jogador_t *j);

/* Aplica dano no jogador */
void jogador_receber_dano(jogador_t *j, int dano);

/* Aplica dano e empurra o jogador pra longe da origem */
void jogador_receber_dano_knockback(jogador_t *j, int dano, int origem_x);

/* Retorna o centro do jogador */
void jogador_centro(const jogador_t *j, int *cx, int *cy);

/* Detecta cliques de tiro (transicao de nao-pressionado pra pressionado) */
void jogador_atualizar_entrada_tiro(jogador_t *j, int *fire_clique, int *fire_forte_clique);

/* Atualiza fisica, colisao e movimento do jogador */
void jogador_atualizar(jogador_t *j, const cenario_t *c, int cutscene_mode);

/* Atualiza qual animacao deve tocar com base no estado */
void jogador_atualizar_animacao(jogador_t *j, float dy);

/* Desenha o jogador na tela */
void jogador_desenhar(const jogador_t *j, int camera_x, int camera_y);

/* Processa disparo de tiros na direcao da mira */
void jogador_processar_tiro(jogador_t *j, tiros_t *tiros, int fire_clique, int fire_forte_clique, int centro_x, int centro_y, float dx, float dy);

#endif
