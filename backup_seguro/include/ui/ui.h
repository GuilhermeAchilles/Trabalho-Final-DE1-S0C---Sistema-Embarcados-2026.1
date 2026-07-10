#ifndef UI_H
#define UI_H

#include "framebuffer/framebuffer.h"
#include "personagem/jogador.h"
#include "inimigo/inimigo.h"

typedef enum {
    ICONE_VIDA,
    ICONE_VELOCIDADE,
    ICONE_INSTAKILL,
    ICONE_SUPER_PULO
} tipo_icone_t;

typedef struct {
    int px, py;
    tipo_icone_t tipo;
    int ativo;
} icone_t;

#define MAX_ICONES 16
extern icone_t icones_mapa[MAX_ICONES];

void desenhar_linha_horizontal(int x0, int x1, int y, fb_color_t cor);
void desenhar_triangulo_preenchido(int x0, int y0, int x1, int y1, int x2, int y2, fb_color_t cor);
void desenhar_mira(int cx, int cy, float dx, float dy, fb_color_t cor);
void desenhar_sinal_go(int x, int y, int frame_contador);
void desenhar_vida(int vida, int vida_maxima);
void desenhar_numero_2_digitos(int numero, int x, int y, int escala, fb_color_t cor);
void desenhar_texto(const char *texto, int x, int y, int escala, fb_color_t cor);
void desenhar_icones(int camera_x, int camera_y, int frame_contador);
void desenhar_barras(int tiros_normais, int cooldown_super, int tiros_carregados, int cooldown_forte);

#endif
