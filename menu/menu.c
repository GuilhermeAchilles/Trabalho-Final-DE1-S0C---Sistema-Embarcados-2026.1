#include "menu/menu.h"
#include "framebuffer/framebuffer.h"

int tela_menu(void) {
    int iniciar = 0;
    int frame_counter = 0;
    while (!fb_poll_quit()) {
        for (int y = 0; y < MENU_HEIGHT; y++) {
            for (int x = 0; x < MENU_WIDTH; x++) {
                fb_put_pixel(x, y, menu_bg[y * MENU_WIDTH + x]);
            }
        }
        
        // Blink logic: show for 30 frames, hide for 30 frames
        if ((frame_counter / 30) % 2 == 0) {
            int start_x = (MENU_WIDTH - PRESS_START_WIDTH) / 2;
            int start_y = MENU_HEIGHT - PRESS_START_HEIGHT - 20; // 20 pixels from bottom
            for (int y = 0; y < PRESS_START_HEIGHT; y++) {
                for (int x = 0; x < PRESS_START_WIDTH; x++) {
                    uint16_t color = press_start_data[y * PRESS_START_WIDTH + x];
                    if (color != PRESS_START_CHROMA_KEY) {
                        fb_put_pixel(start_x + x, start_y + y, color);
                    }
                }
            }
        }
        frame_counter++;

        fb_present();
        
        if (frame_counter > 30) {
            if (fb_key_down(FB_KEY_FIRE)) {
                iniciar = 1;
                break;
            }
            if (fb_key_down(FB_KEY_FIRE_FORTE)) {
                iniciar = 0;
                break;
            }
        }
    }
    return iniciar;
}
