#include "colisao/colisao.h"

int colisao_retangulos(retangulo_t a, retangulo_t b) {
    return a.x < b.x + b.largura &&
           a.x + a.largura > b.x &&
           a.y < b.y + b.altura &&
           a.y + a.altura > b.y;
}
