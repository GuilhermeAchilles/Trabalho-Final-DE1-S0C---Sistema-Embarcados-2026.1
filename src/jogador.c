#include "personagem/jogador.h"
#include "framebuffer/framebuffer.h"
#include "personagem/megamem/movimentos/megaman.h"
#include "ui/ui.h"
#include "icones/icone_vida.h"
#include "icones/icone_velocidade.h"
#include "icones/icone_instakill.h"
#include "icones/icone_super_pulo.h"
#include <math.h>
#define PLAYER_SPEED 4
#define TIRO_COOLDOWN 8
#define VIDA_MAXIMA 10
#define GRAVIDADE 0.5f
#define PULO_FORCA 7.5f
#define VEL_MAX_QUEDA 7.0f
#define JOGADOR_BANDA_PES 14
#define TOLERANCIA_DEGRAU 6
static int borda_de_subida(int pressionado_agora, int *pressionado_antes) {
    int subiu = pressionado_agora && !*pressionado_antes;
    *pressionado_antes = pressionado_agora;
    return subiu;
}

static void jogador_reposicionar_no_spawn(jogador_t *j) {
    j->px = j->spawn_x;
    j->py = j->spawn_y;
    j->vel_y = 0.0f;
    j->no_chao = 0;
    j->pulo_duplo_usado = 0;
    j->vida = VIDA_MAXIMA;
    j->estado = ESTADO_IDLE;
    j->buff_velocidade = 0;
    j->buff_instakill = 0;
    j->buff_super_pulo = 0;
    j->tiros_carregados_restantes = 5;
    j->cooldown_recarregar_forte = 0;
    j->tiros_normais_restantes = 15;
    j->cooldown_superaquecimento = 0;
    for (int i=0; i<RASTRO_MAX; i++) j->rastros[i].vida = 0;
    j->rastro_idx = 0;
    j->dano_piscar_frames = 0;
    j->timer_invulnerabilidade = 0;
    j->knockback_vx = 0;
}

void jogador_iniciar(jogador_t *j, int spawn_x, int spawn_y) {
    j->spawn_x = spawn_x;
    j->spawn_y = spawn_y;
    j->direcao = 1;

    /* MegaMan nao tem sprite de idle proprio - fica parado no 1o frame do correr. */
    animacao_iniciar(&j->anim_idle,             megaman_correr_frames,                1,                                          10, 1);
    animacao_iniciar(&j->anim_andar,            megaman_correr_frames,                MEGAMAN_CORRER_FRAME_COUNT,                5,  1);
    animacao_iniciar(&j->anim_pulo,             megaman_pular_frames,                 MEGAMAN_PULAR_FRAME_COUNT,                 5,  1);
    animacao_iniciar(&j->anim_atirar,           megaman_atirar_frames,                MEGAMAN_ATIRAR_FRAME_COUNT,                4,  1);
    animacao_iniciar(&j->anim_atirar_cima,      megaman_atirar_cima_frames,           MEGAMAN_ATIRAR_CIMA_FRAME_COUNT,           4,  1);
    animacao_iniciar(&j->anim_atirar_baixo,     megaman_atirar_baixo_frames,          MEGAMAN_ATIRAR_BAIXO_FRAME_COUNT,          4,  1);
    animacao_iniciar(&j->anim_atirar_diag_cima, megaman_atirar_diagonal_cima_frames,  MEGAMAN_ATIRAR_DIAGONAL_CIMA_FRAME_COUNT,  4,  1);
    animacao_iniciar(&j->anim_atirar_diag_baixo,megaman_atirar_diagonal_baixo_frames, MEGAMAN_ATIRAR_DIAGONAL_BAIXO_FRAME_COUNT, 4,  1);
    animacao_iniciar(&j->anim_morrer,           megaman_morrer_frames,                MEGAMAN_MORRER_FRAME_COUNT,                6,  0);
    j->frame = animacao_frame_atual(&j->anim_idle);

    j->acao_pressionada_antes = 0;
    j->fire_pressionado_antes = 0;
    j->fire_forte_pressionado_antes = 0;
    j->pulo_pressionado_antes = 0;

    j->tiro_simples   = (tipo_tiro_t){ &megaman_tiro_simples_frames[0],   10, 1 };
    j->tiro_carregado = (tipo_tiro_t){ &megaman_tiro_carregado_frames[0], 12, 2 };
    j->cooldown_tiro = 0;
    j->atirando_pose_frames = 0;

    jogador_reposicionar_no_spawn(j);
}

retangulo_t jogador_hitbox(const jogador_t *j) {
    return (retangulo_t){ j->px, j->py, JOGADOR_LARGURA, JOGADOR_ALTURA };
}

void jogador_receber_dano(jogador_t *j, int dano) {
    if (j->timer_invulnerabilidade > 0) return;
    if (dano > 0) {
        j->vida -= dano;
        j->dano_piscar_frames = 15;
    }
    if (j->vida < 0) j->vida = 0;
}

void jogador_receber_dano_knockback(jogador_t *j, int dano, int origem_x) {
    if (j->timer_invulnerabilidade > 0) return;
    jogador_receber_dano(j, dano);
    
    // Ignora inputs e aplica knockback por curto periodo (10 frames)
    j->timer_invulnerabilidade = 10;
    
    // Angulo de 45 graus pra tras (reduzido drasticamente)
    j->vel_y = -3.0f; 
    j->knockback_vx = (j->px < origem_x) ? -1 : 1;
}

void jogador_centro(const jogador_t *j, int *cx, int *cy) {
    *cx = j->px + JOGADOR_LARGURA / 2;
    *cy = j->py + JOGADOR_ALTURA / 2;
}

/* Le os botoes de tiro, detecta cliques (borda de subida) e conta a duracao da pose de atirar. */
void jogador_atualizar_entrada_tiro(jogador_t *j, int *fire_clique, int *fire_forte_clique) {
    *fire_clique       = borda_de_subida(fb_key_down(FB_KEY_FIRE),       &j->fire_pressionado_antes);
    *fire_forte_clique = borda_de_subida(fb_key_down(FB_KEY_FIRE_FORTE), &j->fire_forte_pressionado_antes);

    if (j->atirando_pose_frames > 0) {
        j->atirando_pose_frames--;
    }
}

/* True se a coluna de pixels (linha horizontal y, largura JOGADOR_LARGURA a partir de x)
   tem chao onde pousar: solido sempre, plataforma so quando nao esta atravessando (drop). */
static int linha_tem_chao(const cenario_t *c, int x, int y, int drop_through) {
    for (int dx = 0; dx <= JOGADOR_LARGURA - 1; dx += 4) {
        int tipo = cenario_colisao(c, x + dx, y);
        if (tipo == CENARIO_SOLIDO) return 1;
        if (tipo == CENARIO_PLATAFORMA && !drop_through) return 1;
    }
    int tipo = cenario_colisao(c, x + JOGADOR_LARGURA - 1, y);
    if (tipo == CENARIO_SOLIDO) return 1;
    if (tipo == CENARIO_PLATAFORMA && !drop_through) return 1;
    return 0;
}

/* Menor y (mais alto, primeiro encontrado descendo) com chao sob TODA a largura do
   jogador entre y0 e y1 - nao so uma borda. Isso evita que virar de direcao bem no
   topo de uma rampa (onde a borda esquerda e a direita do corpo ficam em alturas
   bem diferentes) pareca um degrau maior do que realmente e. -1 se nao achar. */
static int chao_sob_jogador(const cenario_t *c, int x, int y0, int y1, int drop_through) {
    for (int y = y0; y <= y1; y++) {
        if (linha_tem_chao(c, x, y, drop_through)) return y;
    }
    return -1;
}

/* Anda pra esquerda/direita.
   No ar, so uma parede solida na faixa dos pes bloqueia (nao gruda em plataforma).
   No chao, ACOMPANHA o relevo (rampas/escadas feitas de solido ou plataforma) usando
   a largura inteira do pe: sobe ou desce ate TOLERANCIA_DEGRAU por passo; degrau maior
   que isso bloqueia como parede. Isso e o que faz o personagem subir/descer rampas
   andando, em vez de simplesmente atravessar plataformas (que nao bloqueiam colisao
   lateral por si so). */
static int jogador_mover_horizontal(jogador_t *j, const cenario_t *c, int drop_through, int cutscene_mode) {
    int passo = 0;
    
    if (j->timer_invulnerabilidade > 0) {
        j->timer_invulnerabilidade--;
        passo = j->knockback_vx;
    } else {
        int vel = j->buff_velocidade > 0 ? PLAYER_SPEED * 2 : PLAYER_SPEED;
        if (cutscene_mode) {
            passo += vel; j->direcao = 1;
        } else {
            if (fb_key_down(FB_KEY_LEFT))  { passo -= vel; j->direcao = -1; }
            if (fb_key_down(FB_KEY_RIGHT)) { passo += vel; j->direcao = 1;  }
        }
    }
    if (passo == 0) return 0;

    int novo_px = j->px + passo;
    int pes_y = j->py + JOGADOR_ALTURA;

    if (!j->no_chao) {
        int borda_x = (passo > 0) ? (novo_px + JOGADOR_LARGURA - 1) : novo_px;
        for (int y = pes_y - JOGADOR_BANDA_PES; y < pes_y; y++) {
            if (cenario_solido(c, borda_x, y)) return 0;
        }
        j->px = novo_px;
        return 1;
    }

    int topo_busca  = pes_y - JOGADOR_BANDA_PES;
    int fundo_busca = pes_y + TOLERANCIA_DEGRAU;
    int chao_y = chao_sob_jogador(c, novo_px, topo_busca, fundo_busca, drop_through);

    if (chao_y == -1) {
        /* beira de um precipicio - anda e deixa a gravidade cuidar da queda */
        j->px = novo_px;
        j->no_chao = 0;
        return 1;
    }
    if (chao_y < pes_y - TOLERANCIA_DEGRAU) {
        return 0; /* parede ou degrau alto demais pra subir andando */
    }

    j->px = novo_px;
    j->py = chao_y - JOGADOR_ALTURA;
    j->vel_y = 0.0f;
    return 1;
}

/* Aplica gravidade, resolve pouso no chao e batida de cabeca no teto. */
static void jogador_fisica_vertical(jogador_t *j, const cenario_t *c, int drop_through) {
    j->vel_y += GRAVIDADE;
    if (j->vel_y > VEL_MAX_QUEDA) j->vel_y = VEL_MAX_QUEDA;

    int novo_py = j->py + (int)j->vel_y;

    if (j->vel_y >= 0.0f) {
        /* caindo: procura o primeiro chao entre os pes atuais e os novos */
        int pes_atual = j->py + JOGADOR_ALTURA;
        int pes_novo  = novo_py + JOGADOR_ALTURA;
        int chao = -1;
        for (int y = pes_atual; y <= pes_novo; y++) {
            if (linha_tem_chao(c, j->px, y, drop_through)) { chao = y; break; }
        }
        if (chao >= 0) {
            j->py = chao - JOGADOR_ALTURA;
            j->vel_y = 0.0f;
            j->no_chao = 1;
        } else {
            j->py = novo_py;
            j->no_chao = 0;
        }
    } else {
        /* subindo: bate a cabeca em teto solido */
        int teto = novo_py;
        int bateu = 0;
        for (int x = j->px; x < j->px + JOGADOR_LARGURA; x += 4) {
            if (cenario_solido(c, x, teto)) { bateu = 1; break; }
        }
        if (bateu) j->vel_y = 0.0f;
        else       j->py = novo_py;
        j->no_chao = 0;
    }
}

static int jogador_na_lava(const jogador_t *j, const cenario_t *c) {
    for (int y = j->py; y < j->py + JOGADOR_ALTURA; y += 6) {
        for (int x = j->px; x < j->px + JOGADOR_LARGURA; x += 6) {
            if (cenario_colisao(c, x, y) == CENARIO_LAVA) return 1;
        }
    }
    return 0;
}

static void jogador_limitar_ao_mundo(jogador_t *j, const cenario_t *c) {
    if (j->px < 0) j->px = 0;
    if (j->px > c->largura - JOGADOR_LARGURA) j->px = c->largura - JOGADOR_LARGURA;
    if (j->py < 0) j->py = 0;
    if (j->py > c->altura - JOGADOR_ALTURA) j->py = c->altura - JOGADOR_ALTURA;
}

/* Decide estado + move o jogador conforme o input e a fisica do cenario. */
void jogador_atualizar(jogador_t *j, const cenario_t *c, int cutscene_mode) {
    int acao_edge = 0;
    if (!cutscene_mode) {
        acao_edge = borda_de_subida(fb_key_down(FB_KEY_ACTION), &j->acao_pressionada_antes);
    }

    if (j->estado == ESTADO_MORRER) {
        /* preso na animacao de morrer até apertar ACAO de novo (revive no spawn - demo) */
        if (acao_edge) {
            jogador_reposicionar_no_spawn(j);
            animacao_reiniciar(&j->anim_morrer);
        }
        return;
    }
    if (acao_edge) {
        j->estado = ESTADO_MORRER;
        return;
    }

    int drop_through = !cutscene_mode && fb_key_down(FB_KEY_DOWN); /* segurar pra baixo atravessa plataformas */
    int movendo = jogador_mover_horizontal(j, c, drop_through, cutscene_mode);

    if (j->buff_velocidade > 0) j->buff_velocidade--;
    if (j->buff_instakill > 0) j->buff_instakill--;
    if (j->buff_super_pulo > 0) j->buff_super_pulo--;
    if (j->dano_piscar_frames > 0) j->dano_piscar_frames--;

    if (j->buff_velocidade > 0 || (j->buff_super_pulo > 0 && !j->no_chao)) {
        if (j->frame) {
            j->rastros[j->rastro_idx].px = j->px;
            j->rastros[j->rastro_idx].py = j->py;
            j->rastros[j->rastro_idx].frame = j->frame;
            j->rastros[j->rastro_idx].direcao = j->direcao;
            j->rastros[j->rastro_idx].vida = 5;
            j->rastros[j->rastro_idx].eh_verde = (j->buff_super_pulo > 0 && !j->no_chao);
            j->rastro_idx = (j->rastro_idx + 1) % RASTRO_MAX;
        }
    }
    for (int i=0; i<RASTRO_MAX; i++) {
        if (j->rastros[i].vida > 0) j->rastros[i].vida--;
    }

    for (int i=0; i<MAX_ICONES; i++) {
        if (!icones_mapa[i].ativo) continue;
        int hit_x = j->px;
        int hit_y = j->py;
        int iw = 32, ih = 32;
        switch(icones_mapa[i].tipo) {
            case ICONE_VIDA: iw = icone_vida_frames[0].width; ih = icone_vida_frames[0].height; break;
            case ICONE_VELOCIDADE: iw = icone_velocidade_frames[0].width; ih = icone_velocidade_frames[0].height; break;
            case ICONE_INSTAKILL: iw = icone_instakill_frames[0].width; ih = icone_instakill_frames[0].height; break;
            case ICONE_SUPER_PULO: iw = icone_super_pulo_frames[0].width; ih = icone_super_pulo_frames[0].height; break;
        }
        if (!(hit_x + JOGADOR_LARGURA <= icones_mapa[i].px || hit_x >= icones_mapa[i].px + iw ||
              hit_y + JOGADOR_ALTURA <= icones_mapa[i].py || hit_y >= icones_mapa[i].py + ih)) {
            icones_mapa[i].ativo = 0;
            switch(icones_mapa[i].tipo) {
                case ICONE_VIDA: j->vida += 2; if (j->vida > VIDA_MAXIMA) j->vida = VIDA_MAXIMA; break;
                case ICONE_VELOCIDADE: j->buff_velocidade = 600; break;
                case ICONE_INSTAKILL: j->buff_instakill = 600; break;
                case ICONE_SUPER_PULO: j->buff_super_pulo = 600; break;
            }
        }
    }

    if (j->no_chao) {
        j->pulo_duplo_usado = 0;
    }

    float forca_pulo = j->buff_super_pulo > 0 ? PULO_FORCA * 1.5f : PULO_FORCA;

    if (borda_de_subida(fb_key_down(FB_KEY_JUMP), &j->pulo_pressionado_antes)) {
        if (j->no_chao) {
            j->vel_y = -forca_pulo;
            j->no_chao = 0;
        } else if (!j->pulo_duplo_usado) {
            j->vel_y = -forca_pulo;
            j->pulo_duplo_usado = 1;
        }
    }

    jogador_fisica_vertical(j, c, drop_through);
    jogador_limitar_ao_mundo(j, c);

    if (jogador_na_lava(j, c) || j->vida <= 0) {
        j->estado = ESTADO_MORRER;
        return;
    }

    /* animacao exibida prioriza atirar > pular > andar/idle */
    if (j->atirando_pose_frames > 0) j->estado = ESTADO_ATIRAR;
    else if (!j->no_chao)            j->estado = ESTADO_PULAR;
    else                             j->estado = movendo ? ESTADO_ANDAR : ESTADO_IDLE;
}

static animacao_t *jogador_escolher_animacao(jogador_t *j, float dy) {
    switch (j->estado) {
        case ESTADO_ANDAR: return &j->anim_andar;
        case ESTADO_PULAR: return &j->anim_pulo;
        case ESTADO_ATIRAR:
            /* pose de tiro conforme a inclinacao da mira */
            if (dy < -0.85f)      return &j->anim_atirar_cima;
            if (dy > 0.85f)       return &j->anim_atirar_baixo;
            if (dy < -0.35f)      return &j->anim_atirar_diag_cima;
            if (dy > 0.35f)       return &j->anim_atirar_diag_baixo;
            return &j->anim_atirar;
        case ESTADO_MORRER: return &j->anim_morrer;
        default:             return &j->anim_idle;
    }
}

void jogador_atualizar_animacao(jogador_t *j, float dy) {
    animacao_t *anim_atual = jogador_escolher_animacao(j, dy);
    animacao_atualizar(anim_atual);
    j->frame = animacao_frame_atual(anim_atual);
}

/* Desenha o sprite alinhado pela base/centro do hitbox, deslocado pela camera. */
void jogador_desenhar(const jogador_t *j, int camera_x, int camera_y) {
    for (int i=0; i<RASTRO_MAX; i++) {
        if (j->rastros[i].vida > 0 && j->rastros[i].frame) {
            int rx = j->rastros[i].px + JOGADOR_LARGURA / 2 - j->rastros[i].frame->width / 2 - camera_x;
            int ry = j->rastros[i].py + JOGADOR_ALTURA - j->rastros[i].frame->height - camera_y;
            if (j->rastros[i].eh_verde) {
                for(int dy=0; dy<j->rastros[i].frame->height; dy+=2) {
                    for(int dx=0; dx<j->rastros[i].frame->width; dx+=2) {
                        fb_put_pixel(rx+dx, ry+dy, fb_rgb(50,255,50));
                    }
                }
            } else {
                sprite_draw(j->rastros[i].frame, rx, ry, j->rastros[i].direcao == -1);
            }
        }
    }
    int draw_x = j->px + JOGADOR_LARGURA / 2 - j->frame->width / 2 - camera_x;
    int draw_y = j->py + JOGADOR_ALTURA - j->frame->height - camera_y;

    float aquecimento_altura = 0.0f;
    float aquecimento_intensidade = 0.0f;

    if (j->cooldown_superaquecimento > 0) {
        aquecimento_altura = 1.0f; 
        aquecimento_intensidade = j->cooldown_superaquecimento / 90.0f; 
    } else {
        aquecimento_altura = (15.0f - j->tiros_normais_restantes) / 15.0f;
        aquecimento_intensidade = 1.0f; 
    }

    int altura_laranja = (int)(j->frame->height * aquecimento_altura);

    for (int y = 0; y < j->frame->height; y++) {
        for (int x = 0; x < j->frame->width; x++) {
            int src_x = (j->direcao == -1) ? (j->frame->width - 1 - x) : x;
            uint16_t pixel = j->frame->pixels[y * j->frame->width + src_x];
            if (pixel != 0xF81F) {
                if (j->dano_piscar_frames > 0 && (j->dano_piscar_frames % 4) < 2) {
                    /* Efeito de piscar em vermelho ao receber dano */
                    int r = (pixel >> 11) & 0x1F;
                    int g = (pixel >> 5) & 0x3F;
                    int b = pixel & 0x1F;
                    
                    r = r + (31 - r) * 0.8f;
                    g = g * 0.2f;
                    b = b * 0.2f;
                    
                    pixel = (r << 11) | (g << 5) | b;
                } else if (y >= j->frame->height - altura_laranja && aquecimento_altura > 0.0f) {
                    int r = (pixel >> 11) & 0x1F;
                    int g = (pixel >> 5) & 0x3F;
                    int b = pixel & 0x1F;
                    
                    float mix = aquecimento_intensidade * 0.6f;
                    r = r + (int)((31 - r) * mix);
                    g = g + (int)((30 - g) * mix);
                    b = b + (int)((0 - b) * mix);
                    
                    pixel = (r << 11) | (g << 5) | b;
                }
                fb_put_pixel(draw_x + x, draw_y + y, pixel);
            }
        }
    }
}

static void calcular_direcao_mira(int cx, int cy, int mx, int my, int direcao, float *dx, float *dy) {
    float vx = (float)(mx - cx);
    float vy = (float)(my - cy);
    float dist = sqrtf(vx * vx + vy * vy);
    *dx = (dist > 0.0f) ? vx / dist : 1.0f;
    *dy = (dist > 0.0f) ? vy / dist : 0.0f;
}

void jogador_processar_tiro(jogador_t *j, tiros_t *tiros, int fire_clique, int fire_forte_clique,
                                    int centro_x, int centro_y, float dx, float dy) {
    static int ticks_sem_atirar = 0;

    if (j->cooldown_recarregar_forte > 0) {
        j->cooldown_recarregar_forte--;
        if (j->cooldown_recarregar_forte == 0) j->tiros_carregados_restantes = 5;
    }
    
    if (j->cooldown_superaquecimento > 0) {
        j->cooldown_superaquecimento--;
        if (j->cooldown_superaquecimento == 0) j->tiros_normais_restantes = 15;
    } else if (j->tiros_normais_restantes < 15 && !fire_clique) {
        if (++ticks_sem_atirar >= 15) {
            j->tiros_normais_restantes++;
            ticks_sem_atirar = 0;
        }
    }

    if (j->cooldown_tiro > 0) {
        j->cooldown_tiro--;
    }
    if (j->cooldown_tiro != 0) return;

    if (fire_forte_clique && j->tiros_carregados_restantes > 0) {
        tiros_disparar(tiros, centro_x, centro_y, dx, dy, &j->tiro_carregado);
        j->tiros_carregados_restantes--;
        if (j->tiros_carregados_restantes == 0) j->cooldown_recarregar_forte = 180;
        j->cooldown_tiro = TIRO_COOLDOWN;
        j->atirando_pose_frames = TIRO_COOLDOWN;
        ticks_sem_atirar = 0;
    } else if (fire_clique && j->tiros_normais_restantes > 0 && j->cooldown_superaquecimento == 0) {
        tiros_disparar(tiros, centro_x, centro_y, dx, dy, &j->tiro_simples);
        j->tiros_normais_restantes--;
        if (j->tiros_normais_restantes == 0) j->cooldown_superaquecimento = 90;
        j->cooldown_tiro = TIRO_COOLDOWN;
        j->atirando_pose_frames = TIRO_COOLDOWN;
        ticks_sem_atirar = 0;
    }
}

/* Um slot de soldado no campo - "ativo" indica se ha um soldado vivo nesse slot agora
   (quando morre e e substituido, o mesmo slot passa a representar outro soldado). */
