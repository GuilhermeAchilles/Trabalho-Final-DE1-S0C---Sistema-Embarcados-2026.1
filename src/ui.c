#include "ui/ui.h"
#include "framebuffer/framebuffer.h"
#include "icones/icone_vida.h"
#include "icones/icone_velocidade.h"
#include "icones/icone_instakill.h"
#include "icones/icone_super_pulo.h"
#include "ui/ui_barra_normal.h"
#include "ui/ui_barra_carregado.h"
#include <math.h>
icone_t icones_mapa[MAX_ICONES];
void desenhar_linha_horizontal(int x0, int x1, int y, fb_color_t cor) {
    if (x0 > x1) { int tmp = x0; x0 = x1; x1 = tmp; }
    for (int x = x0; x <= x1; x++) {
        fb_put_pixel(x, y, cor);
    }
}

/* Ponto onde a reta (x0,y0)-(x1,y1) cruza a linha horizontal y. */
static int intersecao_x(int x0, int y0, int x1, int y1, int y) {
    if (y0 == y1) return x0;
    return x0 + (int)((float)(x1 - x0) * (float)(y - y0) / (float)(y1 - y0));
}

void desenhar_triangulo_preenchido(int x0, int y0, int x1, int y1, int x2, int y2, fb_color_t cor) {
    int ymin = y0;
    if (y1 < ymin) ymin = y1;
    if (y2 < ymin) ymin = y2;

    int ymax = y0;
    if (y1 > ymax) ymax = y1;
    if (y2 > ymax) ymax = y2;

    for (int y = ymin; y <= ymax; y++) {
        int xs[3];
        int n = 0;

        if ((y >= y0 && y <= y1) || (y <= y0 && y >= y1)) xs[n++] = intersecao_x(x0, y0, x1, y1, y);
        if ((y >= y1 && y <= y2) || (y <= y1 && y >= y2)) xs[n++] = intersecao_x(x1, y1, x2, y2, y);
        if ((y >= y2 && y <= y0) || (y <= y2 && y >= y0)) xs[n++] = intersecao_x(x2, y2, x0, y0, y);

        if (n >= 2) {
            int xmin = xs[0], xmax = xs[0];
            for (int i = 1; i < n; i++) {
                if (xs[i] < xmin) xmin = xs[i];
                if (xs[i] > xmax) xmax = xs[i];
            }
            desenhar_linha_horizontal(xmin, xmax, y, cor);
        }
    }
}

/* Triangulo apontando na direcao (dx, dy) - normalizado -, saindo de (cx, cy). */
void desenhar_mira(int cx, int cy, float dx, float dy, fb_color_t cor) {
    const int distancia = 40; /* quao longe a ponta fica do centro */
    const int tamanho    = 3; /* "largura" da base do triangulo */

    float perp_x = -dy;
    float perp_y = dx;

    int ponta_x = cx + (int)(dx * distancia);
    int ponta_y = cy + (int)(dy * distancia);

    int base1_x = cx + (int)(dx * (distancia - tamanho) + perp_x * tamanho);
    int base1_y = cy + (int)(dy * (distancia - tamanho) + perp_y * tamanho);
    int base2_x = cx + (int)(dx * (distancia - tamanho) - perp_x * tamanho);
    int base2_y = cy + (int)(dy * (distancia - tamanho) - perp_y * tamanho);

    desenhar_triangulo_preenchido(ponta_x, ponta_y, base1_x, base1_y, base2_x, base2_y, cor);
}

/* Uma fileira de quadrados no canto superior esquerdo - um por vida, apagado quando perdida. */
void desenhar_vida(int vida, int vida_maxima) {
    const int tamanho = 8;
    const int espaco  = 2;
    const int margem  = 4;

    for (int i = 0; i < vida_maxima; i++) {
        int x0 = margem + i * (tamanho + espaco);
        fb_color_t cor = (i < vida) ? fb_rgb(220, 30, 30) : fb_rgb(60, 60, 60);

        for (int y = margem; y < margem + tamanho; y++) {
            desenhar_linha_horizontal(x0, x0 + tamanho - 1, y, cor);
        }
    }
}

/* Fonte numerica minima (3x5 pixels por digito) - o projeto ainda nao tem um sistema
   de texto/fonte de verdade, entao os digitos do contador de inimigos sao desenhados
   direto como blocos, no mesmo estilo dos outros desenhos primitivos deste arquivo. */
static const uint8_t FONTE_DIGITOS[10][5] = {
    {0b111, 0b101, 0b101, 0b101, 0b111}, /* 0 */
    {0b010, 0b110, 0b010, 0b010, 0b111}, /* 1 */
    {0b111, 0b001, 0b111, 0b100, 0b111}, /* 2 */
    {0b111, 0b001, 0b111, 0b001, 0b111}, /* 3 */
    {0b101, 0b101, 0b111, 0b001, 0b001}, /* 4 */
    {0b111, 0b100, 0b111, 0b001, 0b111}, /* 5 */
    {0b111, 0b100, 0b111, 0b101, 0b111}, /* 6 */
    {0b111, 0b001, 0b001, 0b001, 0b001}, /* 7 */
    {0b111, 0b101, 0b111, 0b101, 0b111}, /* 8 */
    {0b111, 0b101, 0b111, 0b001, 0b111}, /* 9 */
};

static void desenhar_digito(int digito, int x, int y, int escala, fb_color_t cor) {
    for (int linha = 0; linha < 5; linha++) {
        uint8_t bits = FONTE_DIGITOS[digito][linha];
        for (int col = 0; col < 3; col++) {
            if (!(bits & (1 << (2 - col)))) continue;
            for (int sy = 0; sy < escala; sy++) {
                desenhar_linha_horizontal(x + col * escala, x + col * escala + escala - 1, y + linha * escala + sy, cor);
            }
        }
    }
}

/* Numero de 0 a 99, sempre com 2 digitos (zero a esquerda se precisar). */
void desenhar_numero_2_digitos(int numero, int x, int y, int escala, fb_color_t cor) {
    if (numero < 0) numero = 0;
    if (numero > 99) numero = 99;

    int largura_digito = 3 * escala;
    int espaco = escala;

    desenhar_digito(numero / 10, x, y, escala, cor);
    desenhar_digito(numero % 10, x + largura_digito + espaco, y, escala, cor);
}

/* Registra a borda de subida de um botao/tecla (true so no frame em que ele acabou de ser pressionado). */
void desenhar_icones(int camera_x, int camera_y, int frame_contador) {
    for (int i = 0; i < MAX_ICONES; i++) {
        if (!icones_mapa[i].ativo) continue;
        int tela_x = icones_mapa[i].px - camera_x;
        int tela_y = icones_mapa[i].py - camera_y;
        
        /* faz o icone flutuar levemente */
        int flutuar = (int)(sin(frame_contador * 0.1f) * 3.0f);
        tela_y += flutuar;

        const sprite_frame_t *frame = NULL;
        switch(icones_mapa[i].tipo) {
            case ICONE_VIDA: frame = &icone_vida_frames[0]; break;
            case ICONE_VELOCIDADE: frame = &icone_velocidade_frames[0]; break;
            case ICONE_INSTAKILL: frame = &icone_instakill_frames[0]; break;
            case ICONE_SUPER_PULO: frame = &icone_super_pulo_frames[0]; break;
        }
        if (frame) {
            float escala_x = cos(frame_contador * 0.05f);
            int largura_atual = (int)(frame->width * fabs(escala_x));
            if (largura_atual < 1) largura_atual = 1;
            int offset_x = (frame->width - largura_atual) / 2;
            
            for (int y = 0; y < frame->height; y++) {
                for (int x = 0; x < largura_atual; x++) {
                    int src_x = (int)(x / fabs(escala_x));
                    if (escala_x < 0) src_x = frame->width - 1 - src_x;
                    if (src_x >= frame->width) src_x = frame->width - 1;
                    
                    uint16_t pixel = frame->pixels[y * frame->width + src_x];
                    if (pixel != 0xF81F) {
                        fb_put_pixel(tela_x + offset_x + x, tela_y + y, pixel);
                    }
                }
            }
        }
    }
}

static void desenhar_barra_ui(const sprite_frame_t *frame, int dx, int dy, int fill_w, fb_color_t fill_color) {
    for (int y = 0; y < frame->height; y++) {
        for (int x = 0; x < frame->width; x++) {
            int src_x = frame->width - 1 - x; /* espelhar a barra */
            uint16_t pixel = frame->pixels[y * frame->width + src_x];
            
            if (pixel != 0xF81F) {
                int orig_r = (pixel >> 11) & 0x1F;
                int orig_g = (pixel >> 5) & 0x3F;
                int orig_b = pixel & 0x1F;
                
                int lum = orig_r + orig_g/2 + orig_b; 
                
                if (x < fill_w) {
                    int f_r = (fill_color >> 11) & 0x1F;
                    int f_g = (fill_color >> 5) & 0x3F;
                    int f_b = fill_color & 0x1F;
                    
                    int out_r = (f_r * lum) / 31;
                    int out_g = (f_g * lum) / 31;
                    int out_b = (f_b * lum) / 31;
                    
                    if (out_r > 31) out_r = 31;
                    if (out_g > 63) out_g = 63;
                    if (out_b > 31) out_b = 31;
                    
                    pixel = (out_r << 11) | (out_g << 5) | out_b;
                } else {
                    int gray = lum / 3;
                    if (gray > 31) gray = 31;
                    pixel = (gray << 11) | ((gray * 2) << 5) | gray;
                }
                fb_put_pixel(dx + x, dy + y, pixel);
            }
        }
    }
}

void desenhar_barras(int tiros_normais, int cooldown_super, int tiros_carregados, int cooldown_forte) {
    int normal_x = 4;
    int normal_y = FB_HEIGHT - ui_barra_normal_frames[0].height - 4;
    
    int carregado_x = 4;
    int carregado_y = normal_y - ui_barra_carregado_frames[0].height - 4;
    
    int normal_w = ui_barra_normal_frames[0].width;
    int fill_normal_w = 0;
    if (cooldown_super > 0) {
        fill_normal_w = (int)((1.0f - (cooldown_super / 90.0f)) * normal_w);
    } else {
        fill_normal_w = (int)((tiros_normais / 15.0f) * normal_w);
    }
    
    desenhar_barra_ui(&ui_barra_normal_frames[0], normal_x, normal_y, fill_normal_w, fb_rgb(163, 115, 62));
    
    int carregado_w = ui_barra_carregado_frames[0].width;
    int fill_c_w = 0;
    if (cooldown_forte > 0) {
        fill_c_w = (int)((1.0f - (cooldown_forte / 180.0f)) * carregado_w);
    } else {
        fill_c_w = (int)((tiros_carregados / 5.0f) * carregado_w);
    }
    
    desenhar_barra_ui(&ui_barra_carregado_frames[0], carregado_x, carregado_y, fill_c_w, fb_rgb(40, 125, 237));
}

static const uint8_t FONTE_LETRAS[256][5] = {
    ['A'] = {0b010, 0b101, 0b111, 0b101, 0b101},
    ['B'] = {0b110, 0b101, 0b110, 0b101, 0b110},
    ['C'] = {0b011, 0b100, 0b100, 0b100, 0b011},
    ['D'] = {0b110, 0b101, 0b101, 0b101, 0b110},
    ['E'] = {0b111, 0b100, 0b110, 0b100, 0b111},
    ['F'] = {0b111, 0b100, 0b110, 0b100, 0b100},
    ['G'] = {0b011, 0b100, 0b101, 0b101, 0b011},
    ['H'] = {0b101, 0b101, 0b111, 0b101, 0b101},
    ['I'] = {0b111, 0b010, 0b010, 0b010, 0b111},
    ['J'] = {0b001, 0b001, 0b001, 0b101, 0b010},
    ['K'] = {0b101, 0b101, 0b110, 0b101, 0b101},
    ['L'] = {0b100, 0b100, 0b100, 0b100, 0b111},
    ['M'] = {0b111, 0b111, 0b111, 0b101, 0b101},
    ['N'] = {0b111, 0b101, 0b101, 0b101, 0b101},
    ['O'] = {0b111, 0b101, 0b101, 0b101, 0b111},
    ['P'] = {0b111, 0b101, 0b111, 0b100, 0b100},
    ['Q'] = {0b010, 0b101, 0b101, 0b011, 0b001},
    ['R'] = {0b110, 0b101, 0b110, 0b101, 0b101},
    ['S'] = {0b011, 0b100, 0b010, 0b001, 0b110},
    ['T'] = {0b111, 0b010, 0b010, 0b010, 0b010},
    ['U'] = {0b101, 0b101, 0b101, 0b101, 0b111},
    ['V'] = {0b101, 0b101, 0b101, 0b101, 0b010},
    ['W'] = {0b101, 0b101, 0b111, 0b111, 0b111},
    ['X'] = {0b101, 0b101, 0b010, 0b101, 0b101},
    ['Y'] = {0b101, 0b101, 0b010, 0b010, 0b010},
    ['Z'] = {0b111, 0b001, 0b010, 0b100, 0b111},
    [' '] = {0b000, 0b000, 0b000, 0b000, 0b000},
    ['-'] = {0b000, 0b000, 0b111, 0b000, 0b000},
    ['>'] = {0b100, 0b010, 0b001, 0b010, 0b100},
};

void desenhar_texto(const char *texto, int x, int y, int escala, fb_color_t cor) {
    int cur_x = x;
    while (*texto) {
        char c = *texto++;
        if (c >= 'a' && c <= 'z') c -= 32; // upper
        for (int linha = 0; linha < 5; linha++) {
            uint8_t bits = FONTE_LETRAS[(unsigned char)c][linha];
            for (int col = 0; col < 3; col++) {
                if (!(bits & (1 << (2 - col)))) continue;
                for (int sy = 0; sy < escala; sy++) {
                    desenhar_linha_horizontal(cur_x + col * escala, cur_x + col * escala + escala - 1, y + linha * escala + sy, cor);
                }
            }
        }
        cur_x += 4 * escala; // 3 width + 1 space
    }
}
