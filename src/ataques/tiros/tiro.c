/* Utilidade: Gerenciador do pool de projeteis, atualizando movimento e colisao de tiros */
#include "ataques/tiros/tiro.h"

/* Zera e inativa todos os tiros do pool no comeco do jogo */
void tiros_iniciar(tiros_t *grupo) {
    for (int i = 0; i < TIRO_MAX; i++) {
        grupo->tiros[i].ativo = 0;
    }
}

/* Procura um slot livre no vetor de tiros e dispara um novo projetil */
void tiros_disparar(tiros_t *grupo, int x, int y, float dx, float dy, const tipo_tiro_t *tipo) {
    for (int i = 0; i < TIRO_MAX; i++) {
        if (!grupo->tiros[i].ativo) {
            grupo->tiros[i].x = (float)x;
            grupo->tiros[i].y = (float)y;
            grupo->tiros[i].dx = dx;
            grupo->tiros[i].dy = dy;
            grupo->tiros[i].tipo = tipo;
            grupo->tiros[i].vida = TIRO_VIDA_FRAMES;
            grupo->tiros[i].ativo = 1;
            return;
        }
    }
}

/* Move todos os tiros pela tela e subtrai o tempo de vida de cada um. Morrem quando acaba a vida. */
void tiros_atualizar(tiros_t *grupo) {
    for (int i = 0; i < TIRO_MAX; i++) {
        tiro_t *t = &grupo->tiros[i];

        if (!t->ativo) continue;

        t->x += t->tipo->velocidade * t->dx;
        t->y += t->tipo->velocidade * t->dy;

        if (--t->vida <= 0) {
            t->ativo = 0;
        }
    }
}

/* Percorre os tiros ativos e os desenha levando em conta a posicao da camera */
void tiros_desenhar(const tiros_t *grupo, int camera_x, int camera_y) {
    for (int i = 0; i < TIRO_MAX; i++) {
        const tiro_t *t = &grupo->tiros[i];
        if (!t->ativo || !t->tipo || !t->tipo->sprite) continue;
        
        int draw_x = (int)t->x - camera_x;
        int draw_y = (int)t->y - camera_y;
        int espelhado = (t->dx < 0.0f) ? 1 : 0;

        sprite_draw(t->tipo->sprite, draw_x, draw_y, espelhado);
    }
}

/* Igual ao desenho comum, mas aplica um filtro de cor (tint). O boss usa tiros do heroi mas com cor roxa! */
void tiros_desenhar_tint(const tiros_t *grupo, int camera_x, int camera_y, uint16_t tint_cor, float mix) {
    for (int i = 0; i < TIRO_MAX; i++) {
        const tiro_t *t = &grupo->tiros[i];
        if (!t->ativo || !t->tipo || !t->tipo->sprite) continue;
        
        int draw_x = (int)t->x - camera_x;
        int draw_y = (int)t->y - camera_y;
        int espelhado = (t->dx < 0.0f) ? 1 : 0;

        sprite_draw_tint(t->tipo->sprite, draw_x, draw_y, espelhado, tint_cor, mix);
    }
}

/* Verifica se algum tiro colidiu com um alvo. Se bater, destrói o tiro e soma o dano pro retorno. */
int tiros_colidir_com(tiros_t *grupo, retangulo_t alvo) {
    int dano_total = 0;

    for (int i = 0; i < TIRO_MAX; i++) {
        tiro_t *t = &grupo->tiros[i];
        if (!t->ativo) continue;

        retangulo_t retangulo_tiro = { (int)t->x, (int)t->y, t->tipo->sprite->width, t->tipo->sprite->height };
        if (colisao_retangulos(retangulo_tiro, alvo)) {
            t->ativo = 0;
            dano_total += t->tipo->dano;
        }
    }

    return dano_total;
}