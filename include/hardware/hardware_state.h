/* Variaveis globais de estado do hardware da DE1-SoC */

#ifndef HARDWARE_STATE_H
#define HARDWARE_STATE_H

/* Variaveis globais para atualizar os perifericos da DE1-SoC */
extern int hw_display_inimigos_restantes; /* vai pro display de 7 segmentos */
extern int hw_leds_vida_personagem;       /* LEDs acesos = vida restante */
extern int hw_jogo_pausado;               /* 1 = chave SW0 levantada (jogo pausado) */

#endif
