#ifndef COLISAO_H
#define COLISAO_H

typedef struct {
    int x;
    int y;
    int largura;
    int altura;
} retangulo_t;

/* Testa se dois retangulos (AABB) se sobrepoem. */
int colisao_retangulos(retangulo_t a, retangulo_t b);

#endif
