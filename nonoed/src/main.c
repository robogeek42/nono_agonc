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

bool showingBox = false;

bool do_loop(int vkey, XY* pcursor);
void drawScreen();

const char spc30[32] = "                              ";
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

    // Draw the screen
    title("Nonogram Editor", 6, 13);

    config.col_gridbg = 7;
    config.col_gridmin = 8;
    draw_grid();

    XY cursor;
    cursor.x = 0;
    cursor.y = 0;
    cursorDraw(&cursor);

    vdp_keyboard_control( 250, 500, getsysvar_keyled() );
    int vkey;
    bool exit = false;

    /* =========================================================
     * MAIN LOOP 
     */
    do {
        vkey = wait_for_any_key_press();
        exit = do_loop(vkey, &cursor);
    } while (vkey != KEY_escape && !exit);

    /* =========================================================
     * DeInit
     */
    kbuf_deinit();

    free(grid);

    vdp_cursor_enable(true);
    vdp_write_at_text_cursor();
    TAB(0,3);
    vdp_set_text_colour(15);
    vdp_set_graphics_colour(0,15);
    return 0; 
}

bool do_loop(int vkey, XY* pcursor)
{
    bool endprog = false;
    switch (vkey) {
        case KEY_DOWN:
            cursorClear(pcursor);
            pcursor->y = (pcursor->y + 1) % dims.gs;
            cursorDraw(pcursor);
            break;
        case KEY_UP:
            cursorClear(pcursor);
            pcursor->y = (pcursor->y - 1 + dims.gs) % dims.gs;
            cursorDraw(pcursor);
            break;
        case KEY_RIGHT:
            cursorClear(pcursor);
            pcursor->x = (pcursor->x + 1) % dims.gs;
            cursorDraw(pcursor);
            break;
        case KEY_LEFT:
            cursorClear(pcursor);
            pcursor->x = (pcursor->x - 1 + dims.gs) % dims.gs;
            cursorDraw(pcursor);
            break;
        case KEY_X:
        case KEY_x:
            cursorClear(pcursor);
            set_grid(grid, pcursor, SQ_CROSS);
            redrawGridSquare(grid, pcursor);
            cursorDraw(pcursor);
            calc_column_run(grid, pcursor->x);
            calc_row_run(grid, pcursor->y);
            break;
        case KEY_space:
        case KEY_M:
        case KEY_m:
            cursorClear(pcursor);
            set_grid(grid, pcursor, SQ_FILL);
            redrawGridSquare(grid, pcursor);
            cursorDraw(pcursor);
            calc_column_run(grid, pcursor->x);
            calc_row_run(grid, pcursor->y);
            break;
        case KEY_delete:
        case KEY_D:
        case KEY_d:
            cursorClear(pcursor);
            set_grid(grid, pcursor, SQ_EMPTY);
            redrawGridSquare(grid, pcursor);
            cursorDraw(pcursor);
            calc_column_run(grid, pcursor->x);
            calc_row_run(grid, pcursor->y);
            break;
        case KEY_F:
        case KEY_f:
            cursorClear(pcursor);
            XY pos;
            for (pos.y=0;pos.y<dims.gs;pos.y++) {
                for (pos.x=0;pos.x<dims.gs;pos.x++) {
                    if (get_grid(grid, &pos) == SQ_EMPTY) {
                        set_grid(grid, &pos, SQ_CROSS);
                        redrawGridSquare(grid, &pos);
                    }
                }
            }
            cursorDraw(pcursor);
            for (pos.x=0;pos.x<dims.gs;pos.x++) {
                calc_column_run(grid, pos.x);
            }
            for (pos.y=0;pos.y<dims.gs;pos.y++) {
                calc_row_run(grid, pos.y);
            }
            break;
        case KEY_C:
        case KEY_c:
            if (areYouSure("CLEAR: Are you sure?"))
            {
                cursorClear(pcursor);
                XY pos;
                memset(grid, 0, dims.gs * dims.gs);
                refreshBoard(grid);
                pcursor->x = 0;
                pcursor->y = 0;
                cursorDraw(pcursor);
            }
            break;
        case KEY_S:
        case KEY_s:
            if (isGridComplete(grid)) {
                const char* msg = "SAVE: Filename? ";
                char filename[30];
                vdp_write_at_text_cursor();
                vdp_set_text_colour(15);
                vdp_set_graphics_colour(0,15);
                TAB(0,1);printf("%s",spc30);
                TAB(0,2);printf("%s",spc30);
                TAB(0,1);printf("%s", msg);
                fgets(&filename[0], 28, stdin);
                if (checkFilename(&filename[0])) {
                    saveBoard(grid, &filename[0]);
                    delay(300); // ms
                    clear_keys();
                }
                vdp_set_text_colour(15);
                vdp_set_graphics_colour(0,15);
                TAB(0,1);printf("%s",spc30);
                TAB(0,2);printf("%s",spc30);
            }
            break;
        case KEY_L:
        case KEY_l:
            {
                const char* msg = "LOAD: Filename? ";
                char filename[30];
                vdp_write_at_text_cursor();
                vdp_set_text_colour(15);
                vdp_set_graphics_colour(0,15);

                TAB(0,1);printf("%s",spc30);
                TAB(0,2);printf("%s",spc30);
                TAB(0,1);printf("%s", msg);
                fgets(&filename[0], 28, stdin);
                if (checkFilename(&filename[0])) {
                    loadBoard(grid, filename);
                    drawScreen();
                    cursorDraw(pcursor);
                    delay(300); // ms
                    clear_keys();
                } else {
                    TAB(0,3);printf("Failed to load\n");
                    delay(300); // ms
                    clear_keys();
                    wait_for_any_key();
                }
                vdp_set_text_colour(15);
                vdp_set_graphics_colour(0,15);
                TAB(0,1);printf("%s",spc30);
                TAB(0,2);printf("%s",spc30);
            }
            break;
        case KEY_Q:
        case KEY_q:
            if (areYouSure("QUIT: Are you sure?")) endprog = true;
            break;
        case KEY_T:
        case KEY_t:
            if (!showingBox) {
                msgBox(20,10, "Hello");
                showingBox = true;
            } else {
                drawScreen();
                cursorDraw(pcursor);
                showingBox = false;
            }
            break;
    }    
    return endprog;
}

void drawScreen() 
{
    CLS;
    title("Nonogram Editor", 6, 13);
    draw_grid();
    refreshBoard(grid);
}
