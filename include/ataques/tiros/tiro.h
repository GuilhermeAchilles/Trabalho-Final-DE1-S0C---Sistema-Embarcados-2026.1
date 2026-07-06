#ifndef TIRO_H
#define TIRO_H

#include "framebuffer/framebuffer.h"
#include "sprite/sprite.h"
#include "colisao/colisao.h"

#define TIRO_MAX        16
#define TIRO_VIDA_FRAMES 200 /* apos tantos frames sem acertar nada, o tiro some (mundo pode ser maior que a tela) */

typedef struct
{
    const sprite_frame_t *sprite; /* formato/desenho do tiro */
    int velocidade;
    int dano;
} tipo_tiro_t;

typedef struct
{
    float x;
    float y;
    float dx;
    float dy;
    int ativo;
    int vida;                     /* frames restantes ate sumir sozinho */
    const tipo_tiro_t *tipo;
} tiro_t;

typedef struct
{
    tiro_t tiros[TIRO_MAX];
} tiros_t;

void tiros_iniciar(tiros_t *grupo);

/* Dispara um tiro saindo de (x, y) na direcao (dx, dy) - vetor normalizado. */
void tiros_disparar(tiros_t *grupo, int x, int y, float dx, float dy, const tipo_tiro_t *tipo);

void tiros_atualizar(tiros_t *grupo);

/* Desenha os tiros deslocados pela camera (camera_x/y = canto da tela no mundo). */
void tiros_desenhar(const tiros_t *grupo, int camera_x, int camera_y);

/* Desativa todo tiro ativo que colidir com alvo e retorna quantos acertaram. */
int tiros_colidir_com(tiros_t *grupo, retangulo_t alvo);

#endif