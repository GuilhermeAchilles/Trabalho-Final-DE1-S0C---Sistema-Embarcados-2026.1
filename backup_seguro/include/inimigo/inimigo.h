#ifndef INIMIGO_H
#define INIMIGO_H

#include "sprite/sprite.h"
#include "animacao/animacao.h"
#include "ataques/tiros/tiro.h"
#include "colisao/colisao.h"
#include "cenario/cenario.h"

/* Inimigo generico: fica parado atirando no alvo em intervalos regulares, tem vida e
   morre depois de tomar dano suficiente. Nao e especifico de nenhum personagem - a
   aparencia/tiro/vida vem dos parametros de inimigo_iniciar. */
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
    int intervalo_tiro;  /* frames de jogo entre cada disparo */
    int cooldown_tiro;

    int vida;
    int flash_frames;
    inimigo_estado_t estado;

    int tempo_decisao;
    int comportamento; /* 0 = seguir, 1 = parado, 2 = fugir */
} inimigo_t;

void inimigo_iniciar(inimigo_t *inimigo, int px, int py, int chao_y, int vida,
                      const sprite_frame_t *idle_frames, int idle_frame_count, int idle_frames_por_sprite,
                      const sprite_frame_t *andar_frames, int andar_frame_count, int andar_frames_por_sprite,
                      const sprite_frame_t *atirar_frames, int atirar_frame_count, int atirar_frames_por_sprite,
                      const sprite_frame_t *morrer_frames, int morrer_frame_count, int morrer_frames_por_sprite,
                      const sprite_frame_t *explosao_frames, int explosao_frame_count, int explosao_frames_por_sprite,
                      const sprite_frame_t *paraquedas_frames, int paraquedas_frame_count, int paraquedas_frames_por_sprite,
                      tipo_tiro_t tiro, int intervalo_tiro);

/* Mira em (alvo_x, alvo_y), dispara quando o cooldown zera e atualiza a animacao.
   Coordenadas em espaco de MUNDO. Depois de morto, so toca a animacao de morrer. */
void inimigo_atualizar(inimigo_t *inimigo, tiros_t *tiros, int alvo_x, int alvo_y, const cenario_t *cenario);

/* Desenha o inimigo deslocado pela camera (camera_x/y = canto da tela no mundo).
   Pisca vermelho por alguns frames logo depois de tomar um hit. */
void inimigo_desenhar(const inimigo_t *inimigo, int camera_x, int camera_y);

/* Retangulo de colisao do inimigo (posicao + tamanho do frame atual), em espaco de mundo. */
retangulo_t inimigo_hitbox(const inimigo_t *inimigo);

/* Tira "dano" pontos de vida e comeca o flash vermelho; ao zerar a vida, comeca a
   animacao de morrer (sem efeito se ja estiver morto). */
void inimigo_receber_dano(inimigo_t *inimigo, int dano);

/* 1 quando a vida zerou E a animacao de morrer ja terminou (parado no ultimo frame). */
int inimigo_esta_morto(const inimigo_t *inimigo);

#endif
