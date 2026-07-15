/* Funcoes de deteccao de colisao (AABB) */

#ifndef COLISAO_H
#define COLISAO_H

/* Retangulo usado pra checar colisao entre objetos do jogo */
typedef struct {
    int x;
    int y;
    int largura;
    int altura;
} retangulo_t;

/* Retorna 1 se os dois retangulos estao se sobrepondo */
int colisao_retangulos(retangulo_t a, retangulo_t b);

#endif
