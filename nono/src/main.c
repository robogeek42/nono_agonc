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
ULENTRY *ulist=NULL, *ulist_current=NULL;

bool doActions(int vkey, XY* pcursor);
void drawScreen();
int loadDialog();
int loadFileInfo(char* puzzles_fname);
bool createGrids(int GS);
void redraw_screen(uint8_t* g);
void checkWin();

bool addUndo(uint8_t x, uint8_t y, uint8_t old_state, uint8_t new_state);
bool deleteList(ULENTRY* ulfrom);
bool undoAction(uint8_t *grid);
bool redoAction(uint8_t *grid);
void printUndoList();

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
            exit = doActions(vkey, &cursor);
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

bool doActions(int vkey, XY* pcursor)
{
    bool endprog = false;

    // prevent key bounce
    delay(300); // ms
    clear_keys();

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
            addUndo(pcursor->x, pcursor->y, get_grid(guess, pcursor), SQ_CROSS);
            set_grid(guess, pcursor, SQ_CROSS);
            redrawGridSquare(guess, pcursor);
            checkWin();
            cursorDraw(pcursor);
            break;
        case KEY_space:
        case KEY_M:
        case KEY_m:
            cursorClear(pcursor);
            addUndo(pcursor->x, pcursor->y, get_grid(guess, pcursor), SQ_FILL);
            set_grid(guess, pcursor, SQ_FILL);
            redrawGridSquare(guess, pcursor);
            checkWin();
            cursorDraw(pcursor);

            break;
        case KEY_delete:
        case KEY_D:
        case KEY_d:
            cursorClear(pcursor);
            addUndo(pcursor->x, pcursor->y, get_grid(guess, pcursor), SQ_EMPTY);
            set_grid(guess, pcursor, SQ_EMPTY);
            redrawGridSquare(guess, pcursor);
            cursorDraw(pcursor);
            break;
        case KEY_C:
        case KEY_c:
            if (areYouSure("CLEAR: Are you sure?"))
            {
                XY pos;
                memset(guess, 0, dims.gs * dims.gs);
                deleteList(ulist);
                ulist = NULL;

                pcursor->x = 0;
                pcursor->y = 0;
            }
            drawScreen();
            cursorDraw(pcursor);
            break;
        case KEY_L:
        case KEY_l:
            {
                // Call load dialog
                int ret = loadDialog();
                if (ret < 0) {
                    printf("Error!\nGOODBYE!\n");
                    return true; // end
                } else if (ret == 0) {
                    printf("GOODBYE!\n");
                    return true; // end
                }
                deleteList(ulist);
                ulist = NULL;

                pcursor->x = 0;
                pcursor->y = 0;
                cursorDraw(pcursor);
            }
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
                cursorDraw(pcursor);
            }
            break;

        case KEY_U:
        case KEY_u:
            undoAction(guess);
            break;

        case KEY_R:
        case KEY_r:
            redoAction(guess);
            break;

        case KEY_backtick:
            break;

        case KEY_T:
        case KEY_t:
            // testing only
            for (int i=0; i<dims.gs*dims.gs; i++) {
                guess[i] = grid[i];
            }
            guess[0] = SQ_EMPTY;
            refreshBoard(guess);
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
        TAB(1,1);
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
            TAB(1,3);printf("Failed to load\n");
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
        TAB(1,3);printf("can't open %s", puzzles_fname);
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

void checkWin()
{
    if (isGridComplete(guess)) {
        if (checkSolution(guess, grid) == true) {
            msgBoxModal(20,11, BRIGHT_GREEN, BRIGHT_YELLOW);
            TAB(4,5);printf("You did it!!");
            wait_for_any_key();
            vdp_reset_viewports();
            vdp_set_text_viewport(0, dims.scrHeightChars, dims.scrWidthChars, 0);
            drawScreen();
        }
    }
}

// Add undo. Create a new node.  
// Nodes can only be added at the current pointer
// 3 situations:
//   1. No list
//   2. Current is at end (next==NULL)
//   3. Current is in mid list (next != NULL)
bool addUndo(uint8_t x, uint8_t y, uint8_t old_state, uint8_t new_state)
{
    //vdp_gcol(0,0); vdp_filled_rectangle(0,8,20*8,2*8);
    //vdp_gcol(0,15); vdp_set_text_colour(15);
    //TAB(1,1); printf("save %d,%d %d %d\n", x, y, old_state, new_state);
    // Type 1 : No list yet
    if (!ulist)
    {
        ulist = (ULENTRY*)malloc(sizeof(ULENTRY));
        if (!ulist)
        {
            vdp_gcol(0,15); vdp_set_text_colour(15);
            printf("malloc error\n");
            wait_for_any_key();
            return false;
        }
        ulist->next = NULL;  // indicates tail
        ulist->prev = NULL;  // indicates head
        ulist->x = x;
        ulist->y = y;
        ulist->old_state = old_state;
        ulist->new_state = new_state;

        ulist_current = ulist;
        return true;
    } 

    // Check ulist_current is set
    if (!ulist_current) {
        vdp_gcol(0,15); vdp_set_text_colour(15);
        printf("error current-ptr is NULL!\n");
        delay(5000);
        clear_keys();
        wait_for_any_key();
        return true;
    }

    // maybe bounce?
    if (ulist_current->x == x &&
            ulist_current->y == y &&
            ulist_current->old_state == old_state &&
            ulist_current->new_state == new_state)
    {
        return true;
    }

    // Type 2 : current is at end of list, append
    if (ulist_current->next == NULL)
    {
        // new entry
        ULENTRY* ul = (ULENTRY*)malloc(sizeof(ULENTRY));
        if (!ul) {
            vdp_gcol(0,15); vdp_set_text_colour(15);
            printf("malloc error\n");
            delay(5000);
            clear_keys();
            wait_for_any_key();
            return false;
        }
        ul->prev = ulist_current;
        ul->next = NULL;
        ul->x = x;
        ul->y = y;
        ul->old_state = old_state;
        ul->new_state = new_state;
        // Move current
        ulist_current->next = ul;
        ulist_current = ul;
        return true;

    }

    // Type 3 : we are somewhere in the middle of the undo list
    if (ulist_current->next != NULL)
    {
        // new entry
        ULENTRY* ul = (ULENTRY*)malloc(sizeof(ULENTRY));
        if (!ul) {
            vdp_gcol(0,15); vdp_set_text_colour(15);
            printf("malloc error\n");
            wait_for_any_key();
            return false;
        }
        ul->prev = ulist_current;
        ul->next = NULL;
        ul->x = x;
        ul->y = y;
        ul->old_state = old_state;
        ul->new_state = new_state;

        // Delete list forward of current
        deleteList(ulist_current->next);
        ulist_current->next = NULL;

        // move current
        ulist_current->next = ul;
        ulist_current = ul;

        return true;
    }

    return false;
}

bool deleteList(ULENTRY* ulfrom)
{
    ULENTRY* ul = ulfrom;
    // walk to end
    while (ul->next != NULL)
    {
        ul = ul->next;
    }

    while (ul != ulfrom)
    {
        // get a ptr to the entry to be deleted
        ULENTRY* uldelete = ul;
        // go back one
        ul = ul->prev;
        // cut off the delete entry from the list
        ul->next = NULL;
        // and delete
        free(uldelete);
    }
    return true;
}

bool undoAction(uint8_t *g)
{
    if (!ulist) return false;

    if (!ulist_current) return false;

    //vdp_gcol(0,0); vdp_filled_rectangle(0,8,20*8,2*8);
    //vdp_gcol(0,15); vdp_set_text_colour(15);
    //TAB(1,1); printf("undo %d,%d %d %d\n", ulist_current->x, ulist_current->y, ulist_current->old_state, ulist_current->new_state);

    // redraw to the saved state
    XY upos;
    upos.x = ulist_current->x;
    upos.y = ulist_current->y;
    set_grid(g, &upos, ulist_current->old_state);
    redrawGridSquare(g, &upos);

    // move the undo current pointer to the previous entry
    ulist_current = ulist_current->prev;
    return true;
}

bool redoAction(uint8_t *g)
{
    ULENTRY *ul;

    if (!ulist) return false;
    if (!ulist_current) {
        // at head
        ul = ulist;
    } else {
        // move the undo current pointer to the next entry
        ul = ulist_current->next;
    }

    // at end of undo list
    if (!ul) return true;

    //vdp_gcol(0,0); vdp_filled_rectangle(0,8,20*8,2*8);
    //vdp_gcol(0,15); vdp_set_text_colour(15);
    //TAB(1,1); printf("redo %d,%d %d %d\n", ul->x, ul->y, ul->old_state, ul->new_state);

    // redraw the saved state
    XY upos;
    upos.x = ul->x;
    upos.y = ul->y;
    set_grid(g, &upos, ul->new_state);
    redrawGridSquare(g, &upos);
    // move the current pointer
    ulist_current = ul;

    return true;
}

void printUndoList()
{
    TAB(1,3);
    ULENTRY* ul = ulist;
    vdp_gcol(0,15);
    vdp_set_text_colour(15);
    while(ul!=NULL)
    {
        printf("%p: %d,%d : %s->%s N:%p P:%p\n", ul,
                ul->x, ul->y,
                ul->old_state==SQ_EMPTY?"BLANK":ul->old_state==SQ_CROSS?"CROSS":"FILL",
                ul->new_state==SQ_EMPTY?"BLANK":ul->new_state==SQ_CROSS?"CROSS":"FILL",
                ul->next, ul->prev);
        ul = ul->next;
    }
    printf("----------------------------\n");
}
