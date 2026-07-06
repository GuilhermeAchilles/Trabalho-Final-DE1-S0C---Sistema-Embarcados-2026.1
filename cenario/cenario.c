#include "cenario/cenario.h"

void cenario_iniciar(cenario_t *c, const uint16_t *bg, const uint16_t *fg,
                     const uint8_t *colisao, int largura, int altura) {
    c->bg = bg;
    c->fg = fg;
    c->colisao = colisao;
    c->largura = largura;
    c->altura = altura;
    c->camera_x = 0;
    c->camera_y = 0;
}

int cenario_colisao(const cenario_t *c, int x, int y) {
    if (x < 0 || y < 0 || x >= c->largura || y >= c->altura) {
        return CENARIO_SOLIDO;
    }
    return c->colisao[y * c->largura + x];
}

int cenario_solido(const cenario_t *c, int x, int y) {
    return cenario_colisao(c, x, y) == CENARIO_SOLIDO;
}

int cenario_chao_y(const cenario_t *c, int x, int start_y) {
    if (start_y < 0) start_y = 0;
    for (int y = start_y; y < c->altura; y++) {
        int tipo = cenario_colisao(c, x, y);
        if (tipo == CENARIO_SOLIDO || tipo == CENARIO_PLATAFORMA) {
            return y;
        }
    }
    return c->altura - 1;
}

void cenario_centralizar_camera(cenario_t *c, int alvo_x, int alvo_y) {
    int cam_x = alvo_x - FB_WIDTH / 2;
    int cam_y = alvo_y - FB_HEIGHT / 2;

    int max_x = c->largura - FB_WIDTH;
    int max_y = c->altura - FB_HEIGHT;

    if (cam_x < 0) cam_x = 0;
    if (max_x >= 0 && cam_x > max_x) cam_x = max_x;
    if (cam_y < 0) cam_y = 0;
    if (max_y >= 0 && cam_y > max_y) cam_y = max_y;

    c->camera_x = cam_x;
    c->camera_y = cam_y;
}

void cenario_desenhar_bg(const cenario_t *c) {
    for (int sy = 0; sy < FB_HEIGHT; sy++) {
        int wy = c->camera_y + sy;
        if (wy < 0 || wy >= c->altura) continue;
        int linha = wy * c->largura;

        for (int sx = 0; sx < FB_WIDTH; sx++) {
            int wx = c->camera_x + sx;
            if (wx < 0 || wx >= c->largura) continue;
            fb_put_pixel(sx, sy, c->bg[linha + wx]);
        }
    }
}

void cenario_desenhar_fg(const cenario_t *c) {
    if (c->fg == NULL) return;
    for (int sy = 0; sy < FB_HEIGHT; sy++) {
        int wy = c->camera_y + sy;
        if (wy < 0 || wy >= c->altura) continue;
        int linha = wy * c->largura;

        for (int sx = 0; sx < FB_WIDTH; sx++) {
            int wx = c->camera_x + sx;
            if (wx < 0 || wx >= c->largura) continue;
            
            uint16_t frente = c->fg[linha + wx];
            if (frente != 0x0000) {
                fb_put_pixel(sx, sy, frente);
            }
        }
    }
}
