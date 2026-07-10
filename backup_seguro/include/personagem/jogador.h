#ifndef JOGADOR_H
#define JOGADOR_H

#include "sprite/sprite.h"
#include "animacao/animacao.h"
#include "cenario/cenario.h"
#include "ataques/tiros/tiro.h"
#include "colisao/colisao.h"

#define JOGADOR_LARGURA 20
#define JOGADOR_ALTURA  46

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

    animacao_t anim_idle;
    animacao_t anim_andar;
    animacao_t anim_pulo;
    animacao_t anim_morrer;
    animacao_t anim_atirar;
    animacao_t anim_atirar_cima;
    animacao_t anim_atirar_baixo;
    animacao_t anim_atirar_diag_cima;
    animacao_t anim_atirar_diag_baixo;

    int acao_pressionada_antes;
    int fire_pressionado_antes;
    int fire_forte_pressionado_antes;
    int pulo_pressionado_antes;

    tipo_tiro_t tiro_simples;
    tipo_tiro_t tiro_carregado;
    int cooldown_tiro;
    int atirando_pose_frames;

    int tiros_carregados_restantes;
    int cooldown_recarregar_forte;
    int tiros_normais_restantes;
    int cooldown_superaquecimento;

    int vida;
    int buff_velocidade;
    int buff_instakill;
    int buff_super_pulo;
    rastro_t rastros[RASTRO_MAX];
    int rastro_idx;

    int dano_piscar_frames;
} jogador_t;

void jogador_iniciar(jogador_t *j, int spawn_x, int spawn_y);
retangulo_t jogador_hitbox(const jogador_t *j);
void jogador_receber_dano(jogador_t *j, int dano);
void jogador_centro(const jogador_t *j, int *cx, int *cy);
void jogador_atualizar_entrada_tiro(jogador_t *j, int *fire_clique, int *fire_forte_clique);
void jogador_atualizar(jogador_t *j, const cenario_t *c, int cutscene_mode);
void jogador_atualizar_animacao(jogador_t *j, float dy);
void jogador_desenhar(const jogador_t *j, int camera_x, int camera_y);
void jogador_processar_tiro(jogador_t *j, tiros_t *tiros, int fire_clique, int fire_forte_clique, int centro_x, int centro_y, float dx, float dy);

#endif
