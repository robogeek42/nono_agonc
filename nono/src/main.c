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

typedef struct {
    int id;
    int gs;
    char fname[12];
    char title[20];
    char clue[30];
} PUZZINFO; 

const uint8_t screen_mode = 0;

uint8_t* grid;
uint8_t* guess;
PUZZINFO* puzzinfo;

CONFIG config;
DIMS dims;

bool do_loop(int vkey, XY* pcursor);
void drawScreen();
int loadDialog();
int loadFileInfo(char* puzzles_fname);
bool createGrids(int GS);
void redraw_screen(uint8_t* g);

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

    XY cursor;

    // Init a struct to hold config info
    init_config();

    if (loadDialog() > 0)
    {

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

    }
    /* =========================================================
     * DeInit
     */
    kbuf_deinit();

    free(puzzinfo);
    free(grid);

    vdp_cursor_enable(true);
    vdp_write_at_text_cursor();
    TAB(1,38);
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
            set_grid(guess, pcursor, SQ_CROSS);
            redrawGridSquare(guess, pcursor);
            cursorDraw(pcursor);
            //calc_column_run(grid, pcursor->x);
            //calc_row_run(grid, pcursor->y);
            break;
        case KEY_space:
        case KEY_M:
        case KEY_m:
            cursorClear(pcursor);
            set_grid(guess, pcursor, SQ_FILL);
            redrawGridSquare(guess, pcursor);
            cursorDraw(pcursor);
            if (isGridComplete(guess)) {
                if (checkSolution(guess, grid) == true) {
                    TAB(0,2); printf("You did it!");
                }
            }

            break;
        case KEY_delete:
        case KEY_D:
        case KEY_d:
            cursorClear(pcursor);
            set_grid(guess, pcursor, SQ_EMPTY);
            redrawGridSquare(guess, pcursor);
            cursorDraw(pcursor);
            //calc_column_run(grid, pcursor->x);
            //calc_row_run(grid, pcursor->y);
            break;
        case KEY_C:
        case KEY_c:
            if (areYouSure("CLEAR: Are you sure?"))
            {
                XY pos;
                memset(guess, 0, dims.gs * dims.gs);
                pcursor->x = 0;
                pcursor->y = 0;
            }
            drawScreen();
            break;
        case KEY_L:
        case KEY_l:
            // prevent key bounce
            delay(300); // ms
            clear_keys();

            // Call load dialog
            int ret = loadDialog();
            if (ret < 0) {
                printf("Error!\nGOODBYE!\n");
                return true; // end
            } else if (ret == 0) {
                printf("GOODBYE!\n");
                return true; // end
            }

            pcursor->x = 0;
            pcursor->y = 0;
            cursorDraw(pcursor);
            break;

        case KEY_Q:
        case KEY_q:
            if (areYouSure("QUIT: Are you sure?")) {
                endprog = true;
                CLS;
                msgBoxModal(20,11, BRIGHT_RED, BRIGHT_YELLOW);
                TAB(5,4);printf("GOODBYE!");
                vdp_reset_viewports();
                vdp_set_text_viewport(0, dims.scrHeightChars, dims.scrWidthChars, 0);
                TAB(1,36);
            } else {
                drawScreen();
            }
            break;
    }    
    return endprog;
}

void drawScreen() 
{
    CLS;
    title("Nonograms by Robogeek", 14, 11);
    draw_grid();
    refreshBoard(guess);
    refreshCounts(grid);
    viewMini(guess);
}

int loadDialog()
{
    CLS;
    title("LOAD DIALOG", 4, 12);

    int num_puzz = loadFileInfo("data/puzzles.txt");
    if (num_puzz > 0)
    {
        TAB(0,1);
        printf("Select a puzzle (1 - %d)", num_puzz);
        int line = 3; int col = 2;
        for (int i=0; i<num_puzz; i++)
        {
            TAB(col, line);
            printf("%d: %s (%dx%d)", i+1, puzzinfo[i].title, puzzinfo[i].gs, puzzinfo[i].gs);
            line++;
            if (line > 20) {
                line = 3; col += 36;
            }
        }
        
        int sel = input_int(0,24,"Enter file number: ");
        if (sel==0) return 0;

        sel--;

        if (strlen(puzzinfo[sel].fname)>0)
        {
            char filename[30];
            strcpy(filename, "data/");
            strcpy(&(filename[5]), puzzinfo[sel].fname);

            if (!createGrids(puzzinfo[sel].gs))
            {
                printf("\nFailed to create grids\n");
                wait_for_any_key();
                return -1;
            }

            if (!loadBoard(grid, filename))
            {
                printf("\nFailed to load board\n");
                wait_for_any_key();
                return -1;
            }
            drawScreen();

        } else {
            TAB(0,3);printf("Failed to load\n");
            wait_for_any_key();
            return -1;
        }
    } else {
        printf("Failed to load fileinfo\n");;
        wait_for_any_key();
        return -1;
    }
    return num_puzz;
}

// Load up the puzzle info file
int loadFileInfo(char* puzzles_fname)
{
    int num_puzz;
    char buff[80]; char* token;
    FILE* fptr = fopen(puzzles_fname, "r");
    if (!fptr) {
        TAB(0,3);printf("can't open %s", puzzles_fname);
    } else {
        CLS;
        if (!fgets(buff,80,fptr)) return -1;
        num_puzz = atoi(buff);

        if (!puzzinfo) {
            puzzinfo = (PUZZINFO*) calloc(num_puzz, sizeof(PUZZINFO));
            if (!puzzinfo) return -1;
        }

        for (int i=0; i<num_puzz; i++) {
            if (!fgets(buff,80,fptr)) continue;

            token = strtok(buff, ","); if (!token) return -1;
            puzzinfo[i].id = atoi(token);

            token = strtok(NULL, ","); if (!token) return -1;
            puzzinfo[i].gs = atoi(token);

            token = strtok(NULL, ","); if (!token) return -1;
            strcpy(puzzinfo[i].fname, token);

            token = strtok(NULL, ","); if (!token) return -1;
            strcpy(puzzinfo[i].title, token);

            token = strtok(NULL, ","); if (!token) return -1;
            strcpy(puzzinfo[i].clue, token);

            //printf("%d: <%d> <%d> <%s> <%s> <%s>\n",i,puzzinfo[i].id,puzzinfo[i].gs,puzzinfo[i].fname,puzzinfo[i].title,puzzinfo[i].clue);
        }
    }
    return num_puzz;
}


bool createGrids(int GS)
{ 
    // Initialise a struct to hold dimensioning info
    init_dims(GS);

    free(grid);
    free(guess);

    // Create the grid (to hold solution)
    grid = (uint8_t*) calloc(dims.gs * dims.gs, sizeof(uint8_t));
    if (!grid) {
        return false;
    }
    // Create a grid to hold current in progress guesses
    guess = (uint8_t*) calloc(dims.gs * dims.gs, sizeof(uint8_t));
    if (!guess) {
        return false;
    }
    return true;
}
