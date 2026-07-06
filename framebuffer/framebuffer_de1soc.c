#include "framebuffer/framebuffer.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

/* NOTE: physical address/span match the common DE1-SoC HPS "pixel buffer"
   setup (320x240, RGB565). Confirm FB_PHYS_ADDR and FB_SPAN against your
   project's actual VGA/bridge documentation before relying on this. */
#define FB_PHYS_ADDR 0x08000000
#define FB_SPAN      (FB_WIDTH * FB_HEIGHT * sizeof(fb_color_t))

static int mem_fd = -1;
static volatile fb_color_t *fb_ptr = NULL;

void fb_init(void) {
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    fb_ptr = (volatile fb_color_t *)mmap(
        NULL, FB_SPAN, PROT_READ | PROT_WRITE,
        MAP_SHARED, mem_fd, FB_PHYS_ADDR
    );
}

void fb_shutdown(void) {
    munmap((void *)fb_ptr, FB_SPAN);
    close(mem_fd);
}

void fb_clear(fb_color_t color) {
    for (int i = 0; i < FB_WIDTH * FB_HEIGHT; i++) {
        fb_ptr[i] = color;
    }
}

void fb_put_pixel(int x, int y, fb_color_t color) {
    if (x < 0 || x >= FB_WIDTH || y < 0 || y >= FB_HEIGHT) return;
    fb_ptr[y * FB_WIDTH + x] = color;
}

void fb_present(void) {
    /* Single-buffered direct writes: nothing to flush. */
}

int fb_poll_quit(void) {
    return 0;
}

/* =========================================================================
   SISTEMA DE INPUT (TECLADO E MOUSE NO LINUX DA DE1-SOC)
   =========================================================================
   Para capturar o teclado e o mouse via USB no Linux embarcado, você 
   precisa ler os arquivos de dispositivo em /dev/input/.
   
   - Teclado: /dev/input/eventX (use a struct input_event do <linux/input.h>)
   - Mouse: /dev/input/mice (leia blocos de 3 bytes para pegar movimento e clique)
   
   Recomendação: Crie uma thread (pthread) na fb_init() que fica lendo o 
   mouse e o teclado em background, atualizando variáveis globais (mouse_x, 
   mouse_y e um array de teclas_pressionadas). Aqui na fb_key_down e 
   fb_mouse_pos, você só retorna os valores dessas variáveis!
*/

int fb_key_down(fb_key_t key) {
    /* TODO: Ler do array de teclas (ou mapear push buttons GPIO da placa) */
    (void)key;
    return 0;
}

void fb_mouse_pos(int *x, int *y) {
    /* TODO: Retornar as variaveis atualizadas pela thread do /dev/input/mice */
    *x = FB_WIDTH / 2;
    *y = FB_HEIGHT / 2;
}
