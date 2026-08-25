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

uint8_t* grid;

CONFIG config;
DIMS dims;

int main(int argc, char **argv) {
    /* Initialize keyboard buffer to store 16 events (key up and down) */
    kbuf_init(16);

    vdp_mode(screen_mode);

    vdp_cursor_enable(false);
    //vdp_clear_screen();
    vdp_set_pixel_coordinates();
    
    // Get the dimension from the command line for now
    int GS = 15;
    if (argc > 1) {
        int a = atoi(argv[1]);
        if (a <=30 && a >=5 && (a % 5)==0) GS = a;
    }

    // Initialise a struct to hold dimensioning info
    init_dims(GS);

    // And a struct to hold config info
    init_config();

    // Create the grid
    grid = (uint8_t*) calloc(dims.gs * dims.gs, sizeof(uint8_t));
    if (!grid) {
        return -1;
    }

    title("Nonogram Editor", 6, 13);
    TAB(0,1);
    printf("Size %d %dx%d\n", dims.gs, dims.scrWidthChars, dims.scrHeightChars);

    config.col_gridbg = 7;
    config.col_gridmin = 8;
    draw_grid();

    XY cursor;
    cursor.x = 0;
    cursor.y = 0;
    cursorDraw(&cursor);

    int vkey;
    bool xit = false;
    do {
        vkey = wait_for_any_key();
    
        switch (vkey) {
            case KEY_DOWN:
                cursorClear(&cursor);
                cursor.y = (cursor.y + 1) % dims.gs;
                cursorDraw(&cursor);
                break;
            case KEY_UP:
                cursorClear(&cursor);
                cursor.y = (cursor.y - 1 + dims.gs) % dims.gs;
                cursorDraw(&cursor);
                break;
            case KEY_RIGHT:
                cursorClear(&cursor);
                cursor.x = (cursor.x + 1) % dims.gs;
                cursorDraw(&cursor);
                break;
            case KEY_LEFT:
                cursorClear(&cursor);
                cursor.x = (cursor.x - 1 + dims.gs) % dims.gs;
                cursorDraw(&cursor);
                break;
            case KEY_X:
            case KEY_x:
                cursorClear(&cursor);
                set_grid(grid, &cursor, SQ_CROSS);
                redrawGridSquare(grid, &cursor);
                cursorDraw(&cursor);
                calc_column_run(grid, cursor.x);
                break;
            case KEY_space:
            case KEY_M:
            case KEY_m:
                cursorClear(&cursor);
                set_grid(grid, &cursor, SQ_FILL);
                redrawGridSquare(grid, &cursor);
                cursorDraw(&cursor);
                calc_column_run(grid, cursor.x);
                break;
            case KEY_delete:
            case KEY_D:
            case KEY_d:
                cursorClear(&cursor);
                set_grid(grid, &cursor, SQ_EMPTY);
                redrawGridSquare(grid, &cursor);
                cursorDraw(&cursor);
                calc_column_run(grid, cursor.x);
                break;
            case KEY_F:
            case KEY_f:
                cursorClear(&cursor);
                XY pos;
                for (pos.y=0;pos.y<dims.gs;pos.y++) {
                    for (pos.x=0;pos.x<dims.gs;pos.x++) {
                        if (get_grid(grid, &pos) == SQ_EMPTY) {
                            set_grid(grid, &pos, SQ_CROSS);
                            redrawGridSquare(grid, &pos);
                        }
                    }
                }
                cursorDraw(&cursor);
                for (pos.x=0;pos.x<dims.gs;pos.x++) {
                    calc_column_run(grid, pos.x);
                }
                break;
            case KEY_C:
            case KEY_c:
                if (areYouSure("CLEAR: Are you sure?"))
                {
                    cursorClear(&cursor);
                    XY pos;
                    memset(grid, 0, dims.gs * dims.gs);
                    draw_grid();
                    cursor.x = 0;
                    cursor.y = 0;
                    cursorDraw(&cursor);
                }
                break;
            case KEY_Q:
            case KEY_q:
                if (areYouSure("QUIT: Are you sure?")) xit = true;
                break;
        }    
    } while (vkey != KEY_escape && !xit);

    /* Must deinit, or the MOS key event vector is not unset (also frees buffer)  */
    kbuf_deinit();

    vdp_cursor_enable(true);

    vdp_write_at_text_cursor();
    TAB(0,3);
    vdp_set_text_colour(15);
    return 0; 
}


