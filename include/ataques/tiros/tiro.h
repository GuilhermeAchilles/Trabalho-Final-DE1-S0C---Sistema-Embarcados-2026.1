/* Definicoes e controle de projeteis e tiros */

#ifndef TIRO_H
#define TIRO_H

#include "framebuffer/framebuffer.h"
#include "sprite/sprite.h"
#include "colisao/colisao.h"

#define TIRO_MAX        16
#define TIRO_VIDA_FRAMES 200 /* frames ate o tiro sumir se nao acertar nada */

/* Define o tipo do tiro (sprite, velocidade e dano) */
typedef struct
{
    const sprite_frame_t *sprite;
    int velocidade;
    int dano;
} tipo_tiro_t;

/* Um tiro individual ativo no jogo */
typedef struct
{
    float x;
    float y;
    float dx;
    float dy;
    int ativo;
    int vida;              /* frames restantes ate sumir */
    const tipo_tiro_t *tipo;
} tiro_t;

/* Lista com todos os tiros ativos */
typedef struct
{
    tiro_t tiros[TIRO_MAX];
} tiros_t;

void tiros_iniciar(tiros_t *grupo);

/* Cria um tiro na posicao (x, y) indo na direcao (dx, dy) */
void tiros_disparar(tiros_t *grupo, int x, int y, float dx, float dy, const tipo_tiro_t *tipo);

void tiros_atualizar(tiros_t *grupo);

/* Desenha todos os tiros ativos na tela (com offset da camera) */
void tiros_desenhar(const tiros_t *grupo, int camera_x, int camera_y);

void tiros_desenhar_tint(const tiros_t *grupo, int camera_x, int camera_y, uint16_t tint_cor, float mix);

/* Checa se algum tiro acertou o alvo. Retorna quantos acertaram */
int tiros_colidir_com(tiros_t *grupo, retangulo_t alvo);

#endif