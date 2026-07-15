/* Utilidade: Conexao com hardware real: threads, ponteiros DMA para VGA e perifericos no FPGA */
#define _DEFAULT_SOURCE
#include "framebuffer/framebuffer.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <linux/input.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>
#include "hardware/hardware_state.h"
#include <stdlib.h>

/* Mapeamento de memoria direto no hardware do Pixel Buffer VGA na DE1-SoC.
   Lembre-se de confirmar o endereco base no Qsys se a placa mudar! */
#define FB_PHYS_ADDR_FRONT 0xC8000000
#define FB_PHYS_ADDR_BACK  0xC0000000
#define FB_SPAN            (512 * FB_HEIGHT * sizeof(fb_color_t))

#define CHAR_BUF_ADDR      0xC9000000
#define CHAR_BUF_SPAN      0x2000 /* 8KB */

#define LW_BRIDGE_ADDR     0xFF200000
#define LW_BRIDGE_SPAN     0x200000 /* 2MB */

static int mem_fd = -1;
static volatile fb_color_t *fb_buf_front = NULL;
static fb_color_t *shadow_buffer = NULL; /* Shadow buffer alocado na RAM */
static fb_color_t *fb_ptr = NULL; /* Ponteiro ativo aponta pra RAM */

static volatile char *char_ptr = NULL;
static volatile void *lw_ptr = NULL;

/* Thread paralela de input e variaveis compartilhadas (teclado/mouse via /dev/input) */
#define KEY_STATE_SIZE 256
static volatile int key_state[KEY_STATE_SIZE];

/* Botoes do mouse (0=Esq, 1=Dir, 2=Meio) */
static volatile int mouse_buttons = 0;

/* Posição absoluta do mouse, mantida em coordenadas do framebuffer */
static volatile int mouse_x = FB_WIDTH  / 2;
static volatile int mouse_y = FB_HEIGHT / 2;

/* Flag para sinalizar que as threads devem parar */
static volatile int input_running = 0;

/* Threads de input */
static pthread_t kbd_thread;
static pthread_t mouse_thread;
static pthread_t switch_thread;
static int kbd_thread_criada  = 0;
static int mouse_thread_criada = 0;
static int switch_thread_criada = 0;

/* Converte um numero de 0 a 9 para o formato de bits do Display de 7 Segmentos */
static uint32_t dec2seg(int digit) {
    switch(digit) {
        case 0: return 0x3F;
        case 1: return 0x06;
        case 2: return 0x5B;
        case 3: return 0x4F;
        case 4: return 0x66;
        case 5: return 0x6D;
        case 6: return 0x7D;
        case 7: return 0x07;
        case 8: return 0x7F;
        case 9: return 0x6F;
        default: return 0x00;
    }
}

/* Procura no Linux (/dev/input/eventX) o arquivo do teclado USB e retorna o File Descriptor */
static int abrir_teclado(void) {
    char path[64];
    for (int i = 0; i < 32; i++) {
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        /* Checar se o dispositivo suporta EV_KEY (teclado) */
        unsigned long evbits = 0;
        if (ioctl(fd, EVIOCGBIT(0, sizeof(evbits)), &evbits) >= 0) {
            if (evbits & (1 << EV_KEY)) {
                /* Checar se tem teclas alfanuméricas (KEY_A = 30) para
                   diferenciar de botões de mouse/joystick */
                unsigned long keybits[KEY_STATE_SIZE / (8 * sizeof(unsigned long)) + 1];
                memset(keybits, 0, sizeof(keybits));
                if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits) >= 0) {
                    /* KEY_A = 30 → bit 30. Se está setado, é um teclado. */
                    if (keybits[30 / (8 * sizeof(unsigned long))] &
                        (1UL << (30 % (8 * sizeof(unsigned long))))) {
                        /* Mudar para modo bloqueante para a thread */
                        int flags = fcntl(fd, F_GETFL, 0);
                        fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
                        printf("[DE1-SoC] Teclado encontrado: %s\n", path);
                        return fd;
                    }
                }
            }
        }
        close(fd);
    }
    printf("[DE1-SoC] AVISO: Nenhum teclado USB encontrado em /dev/input/eventX\n");
    return -1;
}

/* Thread que roda em background lendo as teclas pressionadas e guardando num array global */
static void *thread_teclado(void *arg) {
    (void)arg;
    int fd = abrir_teclado();
    if (fd < 0) return NULL;

    struct input_event ev;
    while (input_running) {
        ssize_t n = read(fd, &ev, sizeof(ev));
        if (n < (ssize_t)sizeof(ev)) {
            /* Erro ou interrupção — dar uma pausa e tentar de novo */
            if (!input_running) break;
            usleep(10000); /* 10ms */
            continue;
        }
        if (ev.type == EV_KEY && ev.code < KEY_STATE_SIZE) {
            /* value: 0 = solto, 1 = pressionado, 2 = auto-repeat */
            key_state[ev.code] = (ev.value != 0) ? 1 : 0;
        }
    }
    close(fd);
    return NULL;
}

/* Thread que roda em background lendo os eventos do mouse e calculando a coordenada da mira */
static void *thread_mouse(void *arg) {
    (void)arg;
    int fd = open("/dev/input/mice", O_RDONLY);
    if (fd < 0) {
        printf("[DE1-SoC] AVISO: Nao foi possivel abrir /dev/input/mice (%s)\n",
               strerror(errno));
        return NULL;
    }
    printf("[DE1-SoC] Mouse conectado via /dev/input/mice\n");

    unsigned char data[3];
    while (input_running) {
        ssize_t n = read(fd, data, sizeof(data));
        if (n < 3) {
            if (!input_running) break;
            usleep(10000);
            continue;
        }

        /* Botões */
        mouse_buttons = data[0] & 0x07;

        /* Delta X (bit 4 do byte 0 indica sinal negativo) */
        int dx = data[1];
        if (data[0] & 0x10) dx |= 0xFFFFFF00; /* extensão de sinal */

        /* Delta Y (bit 5 do byte 0 indica sinal negativo) — eixo Y invertido */
        int dy = data[2];
        if (data[0] & 0x20) dy |= 0xFFFFFF00;

        /* Atualizar posição absoluta com clamp */
        int new_x = (int)mouse_x + dx;
        int new_y = (int)mouse_y - dy; /* PS/2: Y positivo = pra cima, tela: pra baixo */

        if (new_x < 0) new_x = 0;
        if (new_x >= FB_WIDTH) new_x = FB_WIDTH - 1;
        if (new_y < 0) new_y = 0;
        if (new_y >= FB_HEIGHT) new_y = FB_HEIGHT - 1;

        mouse_x = new_x;
        mouse_y = new_y;
    }
    close(fd);
    return NULL;
}

/* Thread em background que verifica se a chave SW0 da placa foi levantada (para pausar) */
static void *thread_switches(void *arg) {
    while (input_running) {
        if (lw_ptr && lw_ptr != MAP_FAILED) {
            volatile uint32_t *switch_ptr = (volatile uint32_t *)((uint8_t *)lw_ptr + 0x40);
            /* Se a chave 0 (SW0) estiver levantada, pausa o jogo! */
            if ((*switch_ptr) & 0x01) {
                hw_jogo_pausado = 1;
            } else {
                hw_jogo_pausado = 0;
            }
        }
        usleep(50000); /* 20Hz polling */
    }
    return NULL;
}

/* Prepara a FPGA: mapeia os ponteiros de video (memoria) e cria as threads do teclado/mouse */

void fb_init(void) {
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0) {
        perror("[DE1-SoC] Erro fatal: Nao foi possivel abrir /dev/mem (você é root?)");
        exit(1);
    }

    /* Mapear Pixel Buffers (Front) */
    fb_buf_front = (volatile fb_color_t *)mmap(
        NULL, FB_SPAN, PROT_READ | PROT_WRITE,
        MAP_SHARED, mem_fd, FB_PHYS_ADDR_FRONT
    );
    
    if (fb_buf_front == MAP_FAILED) {
        perror("[DE1-SoC] Erro no mmap do pixel buffer VGA");
        fb_ptr = NULL;
        close(mem_fd);
        mem_fd = -1;
        return;
    }

    shadow_buffer = (fb_color_t *)malloc(FB_SPAN);
    if (!shadow_buffer) {
        perror("[DE1-SoC] Erro ao alocar shadow buffer na RAM");
        exit(1);
    }
    fb_ptr = shadow_buffer; /* Desenha tudo na RAM (super rapido) pra evitar flicking */

    /* Mapear LW Bridge (VGA Ctrl, LEDs, Switches, 7-Seg) */
    lw_ptr = mmap(NULL, LW_BRIDGE_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, LW_BRIDGE_ADDR);

    /* Mapear Character Buffer e Limpar Texto do Linux */
    char_ptr = (volatile char *)mmap(NULL, CHAR_BUF_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, CHAR_BUF_ADDR);
    if (char_ptr != MAP_FAILED) {
        memset((void *)char_ptr, ' ', CHAR_BUF_SPAN);
    }

    printf("[DE1-SoC] VGA mapeado! Double Buffer ativo.\n");

    /* Inicializar estado de input */
    memset((void *)key_state, 0, sizeof(key_state));
    mouse_buttons = 0;
    mouse_x = FB_WIDTH  / 2;
    mouse_y = FB_HEIGHT / 2;

    /* Iniciar threads de input em background */
    input_running = 1;

    if (pthread_create(&kbd_thread, NULL, thread_teclado, NULL) == 0) {
        kbd_thread_criada = 1;
    } else {
        perror("[DE1-SoC] Erro ao criar thread do teclado");
    }

    if (pthread_create(&mouse_thread, NULL, thread_mouse, NULL) == 0) {
        mouse_thread_criada = 1;
    } else {
        perror("[DE1-SoC] Erro ao criar thread do mouse");
    }

    if (pthread_create(&switch_thread, NULL, thread_switches, NULL) == 0) {
        switch_thread_criada = 1;
    } else {
        perror("[DE1-SoC] Erro ao criar thread de switches");
    }
}

/* Finaliza as threads, solta a memoria alocada e fecha a comunicacao com a FPGA de forma segura */
void fb_shutdown(void) {
    /* Parar threads de input */
    input_running = 0;

    if (kbd_thread_criada) {
        pthread_cancel(kbd_thread);
        pthread_join(kbd_thread, NULL);
        kbd_thread_criada = 0;
    }
    if (mouse_thread_criada) {
        pthread_cancel(mouse_thread);
        pthread_join(mouse_thread, NULL);
        mouse_thread_criada = 0;
    }
    if (switch_thread_criada) {
        pthread_cancel(switch_thread);
        pthread_join(switch_thread, NULL);
        switch_thread_criada = 0;
    }

    /* Liberar mapeamentos */
    if (fb_buf_front && fb_buf_front != MAP_FAILED) munmap((void *)fb_buf_front, FB_SPAN);
    if (shadow_buffer) free(shadow_buffer);
    if (char_ptr && char_ptr != MAP_FAILED) munmap((void *)char_ptr, CHAR_BUF_SPAN);
    if (lw_ptr && lw_ptr != MAP_FAILED) munmap((void *)lw_ptr, LW_BRIDGE_SPAN);

    if (mem_fd >= 0) {
        close(mem_fd);
    }
    printf("[DE1-SoC] Shutdown completo.\n");
}

/* Pinta o buffer da memoria RAM inteirinho de uma cor so (ex: pra limpar a tela a cada frame) */
void fb_clear(fb_color_t color) {
    if (!fb_ptr) return;
    for (int y = 0; y < FB_HEIGHT; y++) {
        for (int x = 0; x < FB_WIDTH; x++) {
            fb_ptr[y * 512 + x] = color;
        }
    }
}

/* Pinta um unico pixel (x,y) na tela. Origem no canto superior esquerdo */
void fb_put_pixel(int x, int y, fb_color_t color) {
    if (!fb_ptr) return;
    if (x < 0 || x >= FB_WIDTH || y < 0 || y >= FB_HEIGHT) return;
    fb_ptr[y * 512 + x] = color;
}

/* Manda a imagem montada na RAM para o VGA, e tambem atualiza os LEDs e os Displays 7-seg */
void fb_present(void) {
    if (!lw_ptr || lw_ptr == MAP_FAILED) {
        usleep(16000); /* Fallback */
        return;
    }

    /* Atualizar Hardware (LEDs e Displays) */
    volatile uint32_t *led_ptr = (volatile uint32_t *)((uint8_t *)lw_ptr + 0x0);
    volatile uint32_t *hex3_0_ptr = (volatile uint32_t *)((uint8_t *)lw_ptr + 0x20);
    volatile uint32_t *hex5_4_ptr = (volatile uint32_t *)((uint8_t *)lw_ptr + 0x30);

    /* Acender LEDs proporcional à vida do jogador */
    uint32_t led_mask = (1 << hw_leds_vida_personagem) - 1;
    *led_ptr = led_mask;

    /* Atualizar Displays 7-Seg com a quantidade de inimigos */
    int num = hw_display_inimigos_restantes;
    if (num < 0) num = 0;
    if (num > 999999) num = 999999;
    
    uint32_t hex3_0 = 0;
    hex3_0 |= (dec2seg(num % 10)); /* HEX0 */
    hex3_0 |= (dec2seg((num / 10) % 10) << 8); /* HEX1 */
    hex3_0 |= (dec2seg((num / 100) % 10) << 16); /* HEX2 */
    hex3_0 |= (dec2seg((num / 1000) % 10) << 24); /* HEX3 */
    *hex3_0_ptr = hex3_0;
    
    uint32_t hex5_4 = 0;
    hex5_4 |= (dec2seg((num / 10000) % 10)); /* HEX4 */
    hex5_4 |= (dec2seg((num / 100000) % 10) << 8); /* HEX5 */
    *hex5_4_ptr = hex5_4;

    /* Shadow Buffer Copy */
    /* Copiar frame inteiro da RAM para o VGA em burst memory (muito rapido) */
    if (fb_buf_front && shadow_buffer) {
        memcpy((void *)fb_buf_front, (void *)shadow_buffer, FB_SPAN);
    }

    /* Travar framerate em ~60 FPS (16ms) já que removemos o V-Sync via hardware */
    usleep(16000);
}

/* Retorna 1 se o jogador apertou a tecla Esc para sair do jogo */
int fb_poll_quit(void) {
    /* Sem sistema de janelas — sair é via Ctrl+C no terminal.
       Mas podemos usar ESC (KEY_ESC = 1) como saída limpa. */
    return key_state[KEY_ESC];
}

/* Associa os comandos do jogo (ex: PULO) com a tecla fisica do PC ou mouse (ex: ESPACO) */
int fb_key_down(fb_key_t key) {
    switch (key) {
        case FB_KEY_UP:
            return key_state[KEY_UP]    || key_state[KEY_W];
        case FB_KEY_DOWN:
            return key_state[KEY_DOWN]  || key_state[KEY_S];
        case FB_KEY_LEFT:
            return key_state[KEY_LEFT]  || key_state[KEY_A];
        case FB_KEY_RIGHT:
            return key_state[KEY_RIGHT] || key_state[KEY_D];
        case FB_KEY_FIRE:
            /* Botão esquerdo do mouse (bit 0) */
            return (mouse_buttons & 0x01) != 0;
        case FB_KEY_FIRE_FORTE:
            /* Botão direito do mouse (bit 1) */
            return (mouse_buttons & 0x02) != 0;
        case FB_KEY_JUMP:
            return key_state[KEY_SPACE];
        case FB_KEY_ACTION:
            return key_state[KEY_K];
    }
    return 0;
}

/* Retorna a ultima coordenada lida do mouse, pra usar como mira */
void fb_mouse_pos(int *x, int *y) {
    *x = (int)mouse_x;
    *y = (int)mouse_y;
}

