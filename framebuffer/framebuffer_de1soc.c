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

/* =========================================================================
   PIXEL BUFFER VGA — mapeamento de memória direto no hardware
   =========================================================================
   Endereço físico e span conforme configuração padrão do Pixel Buffer
   DE1-SoC (320x240, RGB565). Confirme FB_PHYS_ADDR e FB_SPAN contra a
   configuração real do Qsys/Platform Designer da sua FPGA.             */

#define FB_PHYS_ADDR 0x08000000
#define FB_SPAN      (FB_WIDTH * FB_HEIGHT * sizeof(fb_color_t))

static int mem_fd = -1;
static volatile fb_color_t *fb_ptr = NULL;

/* =========================================================================
   SISTEMA DE INPUT — Thread em background (teclado + mouse USB)
   =========================================================================
   Conforme descrito na documentação (gemini.md):
   - Uma pthread é criada na fb_init()
   - Teclado: lido via /dev/input/eventX (struct input_event)
   - Mouse:   lido via /dev/input/mice   (blocos de 3 bytes)
   - As funções fb_key_down() e fb_mouse_pos() retornam instantaneamente
     os valores das variáveis estáticas atualizadas pela thread.        */

/* --- Estado compartilhado (escrito pela thread, lido pela engine) --- */

/* Mapa de teclas Linux (KEY_*) → estado pressionado/solto.
   O kernel define KEY_MAX (~767), mas 256 cobre todo teclado USB. */
#define KEY_STATE_SIZE 256
static volatile int key_state[KEY_STATE_SIZE];

/* Botões do mouse (bit 0 = esquerdo, bit 1 = direito, bit 2 = meio) */
static volatile int mouse_buttons = 0;

/* Posição absoluta do mouse, mantida em coordenadas do framebuffer */
static volatile int mouse_x = FB_WIDTH  / 2;
static volatile int mouse_y = FB_HEIGHT / 2;

/* Flag para sinalizar que as threads devem parar */
static volatile int input_running = 0;

/* Threads de input */
static pthread_t kbd_thread;
static pthread_t mouse_thread;
static int kbd_thread_criada  = 0;
static int mouse_thread_criada = 0;

/* -------------------------------------------------------------------------
   Descobrir automaticamente o dispositivo de teclado USB
   Percorre /dev/input/eventX testando quais bits de EV são suportados.
   Um teclado reporta EV_KEY.                                           */
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

/* -------------------------------------------------------------------------
   Thread do TECLADO
   Lê struct input_event de /dev/input/eventX continuamente.
   Eventos com type == EV_KEY e code < KEY_STATE_SIZE são usados para
   atualizar o array key_state[].                                       */
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

/* -------------------------------------------------------------------------
   Thread do MOUSE
   Lê /dev/input/mice — protocolo PS/2 simplificado, blocos de 3 bytes:
     byte[0]: botões (bit0=esq, bit1=dir, bit2=meio) + sinais de overflow
     byte[1]: delta X (com sinal, complemento de 2)
     byte[2]: delta Y (com sinal, complemento de 2)

   A posição absoluta é mantida clamped nas coordenadas do framebuffer. */
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

/* =========================================================================
   IMPLEMENTAÇÃO DA API fb_* (framebuffer.h)
   ========================================================================= */

void fb_init(void) {
    /* --- Pixel buffer VGA --- */
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0) {
        perror("[DE1-SoC] Erro ao abrir /dev/mem (rode com sudo!)");
        return;
    }
    fb_ptr = (volatile fb_color_t *)mmap(
        NULL, FB_SPAN, PROT_READ | PROT_WRITE,
        MAP_SHARED, mem_fd, FB_PHYS_ADDR
    );
    if (fb_ptr == MAP_FAILED) {
        perror("[DE1-SoC] Erro no mmap do pixel buffer VGA");
        fb_ptr = NULL;
        close(mem_fd);
        mem_fd = -1;
        return;
    }
    printf("[DE1-SoC] Pixel buffer VGA mapeado em 0x%08X (%d bytes)\n",
           FB_PHYS_ADDR, (int)FB_SPAN);

    /* --- Inicializar estado de input --- */
    memset((void *)key_state, 0, sizeof(key_state));
    mouse_buttons = 0;
    mouse_x = FB_WIDTH  / 2;
    mouse_y = FB_HEIGHT / 2;

    /* --- Iniciar threads de input em background --- */
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
}

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

    /* Liberar pixel buffer */
    if (fb_ptr && fb_ptr != MAP_FAILED) {
        munmap((void *)fb_ptr, FB_SPAN);
    }
    if (mem_fd >= 0) {
        close(mem_fd);
    }
    printf("[DE1-SoC] Shutdown completo.\n");
}

void fb_clear(fb_color_t color) {
    if (!fb_ptr) return;
    for (int i = 0; i < FB_WIDTH * FB_HEIGHT; i++) {
        fb_ptr[i] = color;
    }
}

void fb_put_pixel(int x, int y, fb_color_t color) {
    if (!fb_ptr) return;
    if (x < 0 || x >= FB_WIDTH || y < 0 || y >= FB_HEIGHT) return;
    fb_ptr[y * FB_WIDTH + x] = color;
}

void fb_present(void) {
    /* Single-buffered: pixels já estão na memória VGA, nada a fazer.
       Porém, adicionamos um delay para limitar a ~60 FPS e não
       queimar CPU desnecessariamente no Cortex-A9. */
    usleep(16000); /* ~16ms ≈ 60 FPS */
}

int fb_poll_quit(void) {
    /* Sem sistema de janelas — sair é via Ctrl+C no terminal.
       Mas podemos usar ESC (KEY_ESC = 1) como saída limpa. */
    return key_state[KEY_ESC];
}

/* =========================================================================
   INPUT: Teclado
   Mapeia fb_key_t → key codes Linux (definidos em <linux/input.h>)
   Mesmos controles do backend SDL:
     A/D ou setas = movimento
     Espaço       = pulo
     Mouse esq.   = tiro normal
     Mouse dir.   = tiro forte
     K            = ação (debug)
   ========================================================================= */
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

/* =========================================================================
   INPUT: Mouse
   Retorna a posição absoluta do cursor em coordenadas do framebuffer
   (0..FB_WIDTH-1, 0..FB_HEIGHT-1), atualizada pela thread do mouse.
   ========================================================================= */
void fb_mouse_pos(int *x, int *y) {
    *x = (int)mouse_x;
    *y = (int)mouse_y;
}
