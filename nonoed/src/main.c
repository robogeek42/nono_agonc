/*
  vim:ts=4
  vim:sw=4
*/
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <agon/vdp.h>
#include <agon/keyboard.h>
#include "common.h"

const uint8_t screen_mode = 0;

int main(int argc, char **argv) {
    struct keyboard_event_t e;
    /* Initialize keyboard buffer to store 16 events (key up and down) */
    kbuf_init(16);

    vdp_mode(screen_mode);

    vdp_cursor_enable(false);
    //vdp_clear_screen();
    vdp_set_pixel_coordinates();
    
    struct dims_t dims;

    int GS = 15;
    if (argc > 1) {
        int a = atoi(argv[1]);
        if (a <=30 && a >=5 && (a % 5)==0) GS = a;
    }
    init_dims(&dims, GS);

    struct config_t config;
    init_config(&config);

    title("Nonogram Editor", 6, 13);
    TAB(0,1);
    printf("Size %d %dx%d\n", dims.gs, dims.scrWidthChars, dims.scrHeightChars);

    draw_grid(&dims, &config);

    do {
        // Wait for an event.
        while (!kbuf_poll_event(&e)) {}

        vdp_cursor_tab(0,4);
        printf("VKey: %d   ",e.vkey);    
        //printf("Asci: %c, Vkey: %d State: %s\r\n", e.ascii ? e.ascii : ' ', e.vkey, e.isdown ? "down" : "up");
    } while (e.ascii != 'q');

    /* Must deinit, or the MOS key event vector is not unset (also frees buffer)  */
    kbuf_deinit();

    vdp_cursor_enable(true);

    return 0; 
}


