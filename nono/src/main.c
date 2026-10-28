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
    int state;
} PUZZINFO; 

const uint8_t screen_mode = 0;

uint8_t* grid;
uint8_t* guess;
PUZZINFO* puzzinfo;
// puzzinfo.state 
#define PUZ_NEW 0
#define PUZ_DONE 1
#define PUZ_INPROG 2
int num_puzz=0;

CONFIG config;
DIMS dims;
ULENTRY *ulist=NULL, *ulist_current=NULL;

//uint8_t keystates[NUMKEYSTATES];

int bm_width = 280; int bm_height = 56;
int bms_width = 210; int bms_height = 42;

bool doActions(int vkey, XY* pcursor);
void sayGoodbye();
void drawScreen();
int loadDialogGUI(int cid);
int loadFileInfo(char* puzzles_fname);
int loadUserInfo(char* userinfo_fname);
int writeUserInfo(char *userinfo_fname);
bool createGrids(int GS);
void redraw_screen(uint8_t* g);
void checkWin();

bool addUndo(uint8_t x, uint8_t y, uint8_t old_state, uint8_t new_state);
bool deleteList(ULENTRY* ulfrom);
bool undoAction(uint8_t *grid);
bool replayUndoAction(uint8_t *grid);
void printUndoList();
void helpLine(int vline, char *keystr, char *descstr);
void helpLine2(int vline, char *keystr, char *descstr, char*descstr2);
void showHelp(bool bDrawScreen);
void drawNono(int x, int y, int w, int h);
void openingAnimation();
void debugPrep(int X, int Y, int W, int H, int COL);
void clearMemoMarks();

const char spc30[32] = "                              ";
char sbuff[80];

int holdCount = 0;

int current_puzzid = 0;
   
int main(int argc, char **argv) {
    int vkey=0;

    /* Initialize keyboard buffer to store 16 events (key up and down) */
    kbuf_init(16);
    //openingAnimation();

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
    init_dims(GS);

    XY cursor;

    // Init a struct to hold config info
    init_config();

    // Load the puzzle data and user completed game info
    num_puzz = loadFileInfo("data/puzzles.txt");
    loadUserInfo("user/user.txt");

    // check if there is a saved game
    bool bSavedGame = false;
    bool bSavedGameLoaded = false;
    int savedGameID = 0;
    FILE* fsaveid = fopen(".resumeid","rb");
    if (fsaveid) {
        savedGameID = fgetc(fsaveid); // max 255
        fclose(fsaveid);
        bSavedGame = true;
    }

    bool bExit = false;
    while (!bSavedGameLoaded && current_puzzid==0 && !bExit) 
    {
        while (vkey==0)
        {
            clearKeyBuffer(300);
            CLS;
            title("Nonograms by Robogeek", 14, 11);
            load_bitmap_file("nono.rgb2", bm_width, bm_height, 0);
            load_bitmap_file("nono_small.rgb2", bms_width, bms_height, 1);
            vdp_select_bitmap(0);
            vdp_draw_bitmap((dims.scrWidthPix-bm_width)/2,120);
        
            //msgBoxModal(50,28, BRIGHT_CYAN, BRIGHT_YELLOW);
            setColours(BRIGHT_YELLOW, BLACK);
            centreText("Welcome to Nonograms!", 7);
            setColours(WHITE, BLACK);
            centreText("a picture puzzle game for the Agon", 9);
            setColours(BRIGHT_YELLOW, BLACK);
            centreText("Press H for Help, Q to quit, any other key to start", dims.scrHeightChars-10);
            if (bSavedGame) {
                setColours(BRIGHT_WHITE, BLACK);
                sprintf(sbuff, "[R to resume last save #%d]",savedGameID);
                centreText(sbuff, dims.scrHeightChars-8);
            }

            vkey = wait_for_any_key_press();
            CLS;
            vdp_reset_viewports();
            vdp_set_text_viewport(0, dims.scrHeightChars, dims.scrWidthChars, 0);

            if (vkey == KEY_H || vkey == KEY_h) {
                clearKeyBuffer(300);
                showHelp(false);
                vkey = 0;
            }
            if (vkey == KEY_Q || vkey == KEY_q) {
                bExit = true;
                sayGoodbye();
            }
        }
        if (bSavedGame) {
            if (vkey == KEY_R || vkey == KEY_r) {
                clearKeyBuffer(300);
                // resume saved game
                if (!createGrids(puzzinfo[savedGameID].gs))
                {
                    setColours(BRIGHT_WHITE, BLACK);
                    printf("\nFailed to create grids\n");
                    wait_for_any_key();
                    return -1;
                }
                // load the puzzle
                loadBoard(grid, puzzinfo[savedGameID].fname);
                // and user saved state
                sprintf(sbuff,"user/%04d.save", savedGameID);
                loadBoard(guess, sbuff);
                
                drawScreen();
                bSavedGameLoaded = true;
                current_puzzid = savedGameID;
            }
        }

        clearKeyBuffer(300);
        if (!bSavedGameLoaded && !bExit)
        {
            current_puzzid = loadDialogGUI(current_puzzid);
            if (current_puzzid < 0) {
                // -1 == error. Exit.
                printf("Error\n");
                goto prog_exit;
            }
        }
        clearKeyBuffer(300);
        vkey=0;
    }

    if (bSavedGameLoaded || current_puzzid > 0)
    {
        clearKeyBuffer(300);

        cursor.x = 0;
        cursor.y = 0;
        cursorDraw(&cursor);

        vdp_keyboard_control( 200, 50, getsysvar_keyled() );
        bool exit = false;

        /* =========================================================
         * MAIN LOOP 
         */
        do {
            vkey = wait_for_keypoll();
            exit = doActions(vkey, &cursor);

            // Show Hold Count
            debugPrep(dims.scrWidthChars - 3, 1, 2, 1, BRIGHT_WHITE);
            if (holdCount > 0) {
                printf("%02d",holdCount);
            }
        } while (!exit);

    }
prog_exit:
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

// Change a square
void doMark(XY* pcursor, uint8_t type, bool noCursorChange)
{
    if (!noCursorChange) cursorClear(pcursor);
    addUndo(pcursor->x, pcursor->y, get_grid(guess, pcursor), type);
    set_grid(guess, pcursor, type);
    redrawGridSquare(guess, pcursor);
    if (type==SQ_FILL || type==SQ_CROSS) checkWin();
    if (!noCursorChange) cursorDraw(pcursor);
}

// check if any of the repeatable action keys are still pressed
bool checkRepeatableActions(XY* pcursor)
{
    bool repeatable = false;
    if (getKeyState(KEY_X)==true || getKeyState(KEY_x)==true) {
        doMark(pcursor, SQ_CROSS, false); repeatable=true;
    }
    if (getKeyState(KEY_space)==true || getKeyState(KEY_M)==true || getKeyState(KEY_m)==true) {
        doMark(pcursor, SQ_FILL, false); repeatable=true;
    }
    if (getKeyState(KEY_delete)==true || getKeyState(KEY_D)==true || getKeyState(KEY_d)==true) {
        doMark(pcursor, SQ_EMPTY, false); repeatable=true;
    }
    if (getKeyState(KEY_Z)==true || getKeyState(KEY_z)==true) {
        repeatable=true;
    }
    if (getKeyState(KEY_1)==true) {
        doMark(pcursor, SQ_MEMOA, false); repeatable=true;
    }
    if (getKeyState(KEY_2)==true) {
        doMark(pcursor, SQ_MEMOB, false); repeatable=true;
    }
    return repeatable;
}

// For each type of key execute action
bool doActions(int vkey, XY* pcursor)
{
    bool endprog = false;

    switch (vkey) {
        case KEY_DOWN:
            if (getKeyState(KEY_DOWN)==true)
            {
                cursorClear(pcursor);
                pcursor->y = (pcursor->y + 1) % dims.gs;
                cursorDraw(pcursor);
                if (checkRepeatableActions(pcursor)) {
                    holdCount++;
                } else {
                    holdCount = 0;
                }
            }
            break;
        case KEY_UP:
            if (getKeyState(KEY_UP)==true)
            {
                cursorClear(pcursor);
                pcursor->y = (pcursor->y - 1 + dims.gs) % dims.gs;
                cursorDraw(pcursor);
                if (checkRepeatableActions(pcursor)) {
                    holdCount++;
                } else {
                    holdCount = 0;
                }
            }
            break;
        case KEY_RIGHT:
            if (getKeyState(KEY_RIGHT)==true)
            {
                cursorClear(pcursor);
                pcursor->x = (pcursor->x + 1) % dims.gs;
                cursorDraw(pcursor);
                if (checkRepeatableActions(pcursor)) {
                    holdCount++;
                } else {
                    holdCount = 0;
                }
            }
            break;
        case KEY_LEFT:
            if (getKeyState(KEY_LEFT)==true)
            {
                cursorClear(pcursor);
                pcursor->x = (pcursor->x - 1 + dims.gs) % dims.gs;
                cursorDraw(pcursor);
                if (checkRepeatableActions(pcursor)) {
                    holdCount++;
                } else {
                    holdCount = 0;
                }
            }
            break;
        case KEY_X:
        case KEY_x:
            if (getKeyState(KEY_X)==true || getKeyState(KEY_x)==true)
            {
                doMark(pcursor, SQ_CROSS, false); holdCount=1;
            }
            break;
        case KEY_space:
        case KEY_M:
        case KEY_m:
            if (getKeyState(KEY_space)==true || getKeyState(KEY_M)==true || getKeyState(KEY_m)==true)
            {
                doMark(pcursor, SQ_FILL, false); holdCount=1;
            }
            break;
        case KEY_delete:
        case KEY_D:
        case KEY_d:
            if (getKeyState(KEY_delete)==true || getKeyState(KEY_D)==true || getKeyState(KEY_d)==true)
            {
                doMark(pcursor, SQ_EMPTY, false); holdCount=1;
            }
            break;
        case KEY_1:
            if (getKeyState(KEY_1)==true)
            {
                doMark(pcursor, SQ_MEMOA, false); holdCount=1;
            }
            break;
        case KEY_2:
            if (getKeyState(KEY_2)==true)
            {
                doMark(pcursor, SQ_MEMOB, false); holdCount=1;
            }
            break;
        case KEY_K:
        case KEY_k:
            if (getKeyState(KEY_K)==true || getKeyState(KEY_k)==true)
            {
                cursorClear(pcursor);
                clearMemoMarks();
                cursorDraw(pcursor);
            }
            break;
        case KEY_Z:
        case KEY_z:
            // Z key can be used to count spaces without changing them
            if (getKeyState(KEY_Z)==true || getKeyState(KEY_z)==true)
            {
                if (checkRepeatableActions(pcursor)) {
                    holdCount++;
                } else {
                    holdCount = 0;
                }
            }
            break;
        case KEY_C:
        case KEY_c:
            if (getKeyState(KEY_C)==true || getKeyState(KEY_c)==true)
            {
                clearKeyBuffer(200);

                if (areYouSure("CLEAR: Are you sure?"))
                {
                    XY pos;
                    memset(guess, 0, dims.gs * dims.gs);
                    if (ulist)
                    {
                        deleteList(ulist);
                        ulist = NULL;
                    }

                    pcursor->x = 0;
                    pcursor->y = 0;
                }
                drawScreen();
                cursorDraw(pcursor);
            }
            break;
        case KEY_L:
        case KEY_l:
            if (getKeyState(KEY_L)==true || getKeyState(KEY_l)==true)
            {
                clearKeyBuffer(200);

                // save state
                sprintf(sbuff,"user/%04d.save", current_puzzid);
                saveBoard(guess, sbuff);
                if (puzzinfo[current_puzzid-1].state != PUZ_DONE) puzzinfo[current_puzzid-1].state = PUZ_INPROG;

                // write ID for resume
                FILE *fid = fopen(".resumeid", "wb");
                if (fid)
                {
                    fputc(current_puzzid, fid);
                    fclose(fid);
                }

                // Call load dialog
                CLS;
                title("Nonograms by Robogeek", 14, 11);
                current_puzzid = loadDialogGUI(current_puzzid);
                if (current_puzzid < 0) {
                    printf("Error!\nGOODBYE!\n");
                    return true; // end
                } else if (current_puzzid == 0) {
                    CLS;
                    drawScreen();
                }
                if (ulist)
                {
                    deleteList(ulist);
                    ulist = NULL;
                }

                pcursor->x = 0;
                pcursor->y = 0;
                cursorDraw(pcursor);
                clearKeyBuffer(200);
            }
            break;

        case KEY_Q:
        case KEY_q:
            if (getKeyState(KEY_Q)==true || getKeyState(KEY_q)==true)
            {
                clearKeyBuffer(200);

                if (areYouSure("QUIT: Are you sure?")) {
                    // save 
                    sprintf(sbuff,"user/%04d.save", current_puzzid);
                    saveBoard(guess, sbuff);
                    if (puzzinfo[current_puzzid-1].state != PUZ_DONE) puzzinfo[current_puzzid-1].state = PUZ_INPROG;

                    // write ID for resume
                    FILE *fid = fopen(".resumeid", "wb");
                    if (fid)
                    {
                        fputc(current_puzzid, fid);
                        fclose(fid);
                    }

                    // rewrite user file
                    writeUserInfo("user/user.txt");
                    
                    endprog = true;
                    sayGoodbye();
                } else {
                    drawScreen();
                    cursorDraw(pcursor);
                }
                clearKeyBuffer(200);
            }
            break;

        case KEY_U:
        case KEY_u:
            if (getKeyState(KEY_U)==true || getKeyState(KEY_u)==true)
            {
                undoAction(guess);
            }
            break;

        case KEY_R:
        case KEY_r:
            if (getKeyState(KEY_R)==true || getKeyState(KEY_r)==true)
            {
                replayUndoAction(guess);
            }
            break;

        case KEY_backtick:
            // testing only
            if (getKeyState(KEY_backtick)==true)
            {
                for (int i=0; i<dims.gs*dims.gs; i++) {
                    guess[i] = grid[i];
                }
                guess[0] = SQ_EMPTY;
                refreshBoard(guess);
            }
            break;
        case KEY_H:
        case KEY_h:
            if (getKeyState(KEY_H)==true || getKeyState(KEY_h)==true)
            {
                clearKeyBuffer(300);

                showHelp(true);
                cursorDraw(pcursor);
                clearKeyBuffer(300);
            }
            break;
        case KEY_S:
        case KEY_s:
            if (getKeyState(KEY_S)==true || getKeyState(KEY_s)==true)
            {
                clearKeyBuffer(300);

                // save 
                sprintf(sbuff,"user/%04d.save", current_puzzid);
                saveBoard(guess, sbuff);
                // write ID
                FILE *fid = fopen(".resumeid", "wb");
                if (fid)
                {
                    fputc(current_puzzid, fid);
                    fclose(fid);
                }
                if (puzzinfo[current_puzzid-1].state != PUZ_DONE) puzzinfo[current_puzzid-1].state = PUZ_INPROG;
            }
            break;
    }
    return endprog;
}

void sayGoodbye()
{
    CLS;
    msgBoxModal(20,11, BRIGHT_RED, BRIGHT_YELLOW);
    TAB(5,4);printf("GOODBYE!");
    vdp_reset_viewports();
    vdp_set_text_viewport(0, dims.scrHeightChars, dims.scrWidthChars, 0);
    TAB(1,36);
}

void drawScreen() 
{
    CLS;
    title("Nonograms by Robogeek", 14, 11);
    vdp_select_bitmap(1);
    vdp_draw_bitmap(0,8);
    draw_grid();
    refreshBoard(guess);
    refreshCounts(grid);
    viewMini(guess);
}

void showFinalBitmap(int id, int X, int Y, int MS)
{
    char rgb2fname[24] = "data/";
    int buffid = 4;
    int idx = id - 1;
    strcpy(&rgb2fname[5], puzzinfo[idx].fname);
    rgb2fname[5+7]='r';
    rgb2fname[5+8]='g';
    rgb2fname[5+9]='b';
    rgb2fname[5+10]='2';
    rgb2fname[5+11]=0;
    setColours(BLACK, BLACK);
    vdp_filled_rectangle(X, Y, X+MS-1, Y+MS-1);
    load_bitmap_file(rgb2fname,MS, MS, buffid);
    vdp_move_to(0,0);
    setColours(WHITE, BLACK);
    //printf("FILE: %s\n",rgb2fname);
    vdp_select_bitmap(buffid);
    vdp_draw_bitmap(X, Y);
}

int loadDialogGUI(int cid)
{
    int selid=0;
    if (num_puzz > 0)
    {
        int picgrid_size = 64;
        int picgrid_w = (int)((dims.scrWidthPix - 16) / picgrid_size);
        int picgrid_h = (int)((dims.scrHeightPix - 32) / picgrid_size);
        int pic_row = 0, pic_col = 0;


        // Display a grid of puzzles
        int qbuffid = 2, rbuffid = 3;
        int qbsize = 40;
        int buffid = 4;
        load_bitmap_file("data/q2.rgb2", qbsize, qbsize, qbuffid);
        load_bitmap_file("data/r2.rgb2", qbsize, qbsize, rbuffid);
        char rgb2fname[20] = "data/";
        int C1,C2,MS;
        for (int idx=0; idx<num_puzz; idx++)
        {
            int x = 8 + (pic_col*picgrid_size);
            int y = 16 + (pic_row*picgrid_size);
            switch(puzzinfo[idx].gs)
            {
                case 10:
                    C1 = BLUE;
                    C2 = WHITE;
                    MS = 40;
                    break;
                case 15:
                    C1 = GREEN;
                    C2 = BRIGHT_WHITE;
                    MS = 30;
                    break;
                case 20:
                    C1 = CYAN;
                    C2 = BLACK;
                    MS = 40;
                    break;
                case 25:
                    C1 = MAGENTA;
                    C2 = BLACK;
                    MS = 25;
                    break;
            }
            // colour coded background
            setColours(C1, BLACK);
            vdp_filled_rectangle(x+4,y+12,x+picgrid_size-4,y+picgrid_size-4);

            // number above
            setColours(BRIGHT_YELLOW, BLACK);
            vdp_write_at_graphics_cursor();
            vdp_move_to(x+12, y+4); printf("#%03d",puzzinfo[idx].id);

            if (puzzinfo[idx].state == PUZ_DONE)
            {
                showFinalBitmap(puzzinfo[idx].id, x+(picgrid_size-MS)/2, y+8+(picgrid_size-MS)/2, MS);
            } 
            else if (puzzinfo[idx].state == PUZ_INPROG)
            {
                // Recycle bitmap
                vdp_select_bitmap(rbuffid);
                vdp_draw_bitmap(x+(picgrid_size-qbsize)/2, y+8+(picgrid_size-qbsize)/2);
            }
            else // not started puzzle
            {
                // Q? bitmap    
                vdp_select_bitmap(qbuffid);
                vdp_draw_bitmap(x+(picgrid_size-qbsize)/2, y+8+(picgrid_size-qbsize)/2);
            }

            // Size
            setColours(C2, BLACK);
            vdp_write_at_graphics_cursor();
            vdp_move_to(x+4, y+12); printf("%dx%d",puzzinfo[idx].gs, puzzinfo[idx].gs);
            
            // Next grid square
            pic_col++;
            if (pic_col==picgrid_w) {
                pic_row++;
                pic_col = 0;
            }
        }

        // Interactive GUI - arrow-keys and Enter
        int offx,offy;
        int selx=0, sely=0;
        if (cid>0) {
            selid = cid-1;
            selx = selid % picgrid_w;
            sely = (int) (selid / picgrid_w);
        }
        
        bool selected = false;
        int vkey;
        clearKeyBuffer(300);
        while (!selected)
        {
            offx = 8 + (selx*picgrid_size);
            offy = 16 + (sely*picgrid_size);
            setColours(BRIGHT_RED,BLACK);
            vdp_rectangle(offx,offy,offx+picgrid_size-1,offy+picgrid_size-1);
            vdp_rectangle(offx+1,offy+1,offx+picgrid_size-2,offy+picgrid_size-2);

            vkey = wait_for_keypoll();

            setColours(BLACK,BLACK);
            vdp_rectangle(offx,offy,offx+picgrid_size-1,offy+picgrid_size-1);
            vdp_rectangle(offx+1,offy+1,offx+picgrid_size-2,offy+picgrid_size-2);

            switch (vkey) {
                case KEY_DOWN:
                    if (selid+picgrid_w < num_puzz)
                    {
                        selid+=picgrid_w;
                        sely+=1;
                    }
                    break;
                case KEY_UP:
                    if (selid-picgrid_w >= 0)
                    {
                        selid-=picgrid_w;
                        sely-=1;
                    }
                    break;
                case KEY_LEFT:
                    if (selid > 0)
                    {
                        selid-=1;
                        selx-=1;
                        if (selx<0)
                        {
                            selx=picgrid_w-1;
                            sely-=1;
                        }
                    }
                    break;
                case KEY_RIGHT:
                    if (selid < (num_puzz-1))
                    {
                        selid++;
                        selx+=1;
                        if (selx >= picgrid_w)
                        {
                            selx=0;
                            sely++;
                        }
                    }
                    break;
                case KEY_enter:
                    selected=true;
                    break;
                case KEY_Q:
                case KEY_q:
                    selected=true;
                    selid=-1;
                    break;
            }
            clearKeyBuffer(300);
        }
        if (selid < 0)
        {
            // exit load
            return 0;   // this is a puzzle-id (starts at 1, not the index into the array)
        }
            
        if (strlen(puzzinfo[selid].fname)>0)
        {
            char filename[30];
            strcpy(filename, "data/");
            strcpy(&(filename[5]), puzzinfo[selid].fname);

            if (!createGrids(puzzinfo[selid].gs))
            {
                printf("\nFailed to create grids\n");
                wait_for_any_key();
                return -1;
            }

            // load the board
            if (!loadBoard(grid, filename))
            {
                printf("\nFailed to load board\n");
                wait_for_any_key();
                return -1;
            }
            // and user saved state
            sprintf(sbuff,"user/%04d.save", puzzinfo[selid].id);
            loadBoard(guess, sbuff);
            drawScreen();

        } else {
            TAB(1,3);printf("Failed to load\n");
            wait_for_any_key();
            return -1;
        }

    }
    clearKeyBuffer(300);
    return puzzinfo[selid].id;
}

// Load up the puzzle info file
int loadFileInfo(char* puzzles_fname)
{
    int cnt=0;
    char buff[80]; char* token;
    FILE* fptr = fopen(puzzles_fname, "r");
    if (!fptr) {
        TAB(1,0);printf("can't open %s", puzzles_fname);
    } else {
        //CLS;
        if (!fgets(buff,80,fptr)) return -1;
        cnt = atoi(buff);

        if (!puzzinfo) {
            puzzinfo = (PUZZINFO*) calloc(cnt, sizeof(PUZZINFO));
            if (!puzzinfo) return -1;
        }

        for (int i=0; i<cnt; i++) {
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
    return cnt;
}

int loadUserInfo(char* userinfo_fname)
{
    char buff[40]; char* token = NULL;
    FILE* fptr = fopen(userinfo_fname, "r");
    if (fptr) {
        // each line is a puzzle id and a state
        while (fgets(buff,40,fptr))
        {
            token = strtok(buff, ",");
            int id = atoi(token);
            token = strtok(NULL, ",");
            puzzinfo[id-1].state = atoi(token);
        }
    }
    return 0;
}

int writeUserInfo(char *userinfo_fname)
{
    FILE* fptr = fopen(userinfo_fname, "w");
    if (!fptr) {
        TAB(1,0);printf("can't open %s for write", userinfo_fname);
        return -1;
    } else {
        for (int idx=0; idx<num_puzz; idx++)
        {
            if (puzzinfo[idx].state > 0)
            {
                fprintf(fptr, "%d,%d\n", puzzinfo[idx].id, puzzinfo[idx].state);
            }
        }
        fclose(fptr);
    }
    return 0;
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
            msgBoxModal(30,21, BRIGHT_GREEN, BRIGHT_YELLOW);
            TAB(9,10);printf("You did it!!");
            clearKeyBuffer(300);

            puzzinfo[current_puzzid-1].state = PUZ_DONE;

            // Display completed bitmap
            int MS = 40;
            if (dims.gs == 15) MS=30;
            if (dims.gs == 25) MS=25;
            showFinalBitmap(current_puzzid, 30*8+10, 21*8+16, MS);

            wait_for_any_key();
            vdp_reset_viewports();
            vdp_set_text_viewport(0, dims.scrHeightChars, dims.scrWidthChars, 0);
            clearKeyStates();
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
    //debugPrep(22,1,20,1,15);printf("save %d,%d %d %d\n", x, y, old_state, new_state);
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
        //vdp_gcol(0,15); vdp_set_text_colour(15);
        //printf("error current-ptr is NULL!\n");
        //delay(5000);
        //clear_keys();
        //wait_for_any_key();
        ulist->next = NULL;  // indicates tail
        ulist->prev = NULL;  // indicates head
        ulist->x = x;
        ulist->y = y;
        ulist->old_state = old_state;
        ulist->new_state = new_state;
        ulist_current = ulist;
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

// move back through undo list, undoing the actions
bool undoAction(uint8_t *g)
{
    if (!ulist) {
        //debugPrep(22,1,20,1,9); printf("no ulist");
        return false;
    }

    if (!ulist_current) {
        //debugPrep(22,1,20,1,9); printf("no ulist_current");
        return false;
    }

    //debugPrep(22,1,20,1,15); printf("undo %d,%d %d %d\n", ulist_current->x, ulist_current->y, ulist_current->old_state, ulist_current->new_state);

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

bool replayUndoAction(uint8_t *g)
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
    if (!ul) {
        //debugPrep(22,1,20,1,11); printf("End of list");
        return true;
    }

    //debugPrep(22,1,20,1,15); printf("redo %d,%d %d %d\n", ul->x, ul->y, ul->old_state, ul->new_state);

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

void helpLine(int vline, char *keystr, char *descstr)
{
    int cola=32; int colb=40;
    TAB(cola-strlen(keystr), vline); setColours(BRIGHT_YELLOW, BLACK); printf("%s", keystr);
    setColours(WHITE, BLACK);
    for (int i=cola+2; i<colb; i++) { printf("."); }
    TAB(colb, vline); setColours(YELLOW, BLACK); printf("%s", descstr);
}
void helpLine2(int vline, char *keystr, char *descstr, char*descstr2)
{
    int cola=32; int colb=40;
    TAB(cola-strlen(keystr), vline); setColours(BRIGHT_YELLOW, BLACK); printf("%s", keystr);
    setColours(WHITE, BLACK);
    for (int i=cola+2; i<colb; i++) { printf("."); }
    TAB(colb, vline); setColours(YELLOW, BLACK); printf("%s", descstr);
    setColours(WHITE, BLACK); printf(" %s", descstr2);
}

void showHelp(bool bDrawScreen)
{
    CLS;
    int boxWidth = dims.scrWidthChars -2;
    int boxHeight = dims.scrHeightChars -2;
    msgBoxModal(boxWidth, boxHeight, BRIGHT_CYAN, BRIGHT_YELLOW);

    vdp_select_bitmap(0);
    vdp_draw_bitmap((boxWidth*8-bm_width)/2,16);

    //centreTextInWidth("NONOGRAMS HELP", 5, boxWidth);
    //centreTextInWidth("==============", 6, boxWidth);

    setColours(WHITE, BLACK); // WHITE==light grey
    int vline = 9;

    centreTextInWidth("Nonograms are picture logic puzzles", vline++, boxWidth);
    centreTextInWidth("in which cells in a grid must be colored or left blank", vline++, boxWidth);
    centreTextInWidth("according to numbers at the edges of the grid", vline++, boxWidth);
    centreTextInWidth("to reveal a hidden picture.", vline++, boxWidth); 
    vline++;
    centreTextInWidth("The numbers show how many unbroken lines of", vline++, boxWidth);
    centreTextInWidth("filled-in squares there are in any given", vline++, boxWidth);
    centreTextInWidth("row or column.", vline++, boxWidth);
    vline+=2;

    helpLine(vline, "Arrow keys, Joystick", "Move cursor"); vline+=2;
    helpLine2(vline, "<SPACE>, M",  "Fill a square", "Hold+Move to repeat"); vline+=2;
    helpLine2(vline, "<DEL>, D, X", "Clear a square", "Hold+Move to repeat"); vline+=2;
    helpLine2(vline, "1 / 2",       "Place memo mark", "Hold+move to repeat"); vline+=2;
    helpLine(vline, "Z", "Move and count while holding"); vline+=2;
    setColours(WHITE, BLACK); // WHITE==light grey
    centreTextInWidth("(Repeat count in top-right)", vline++, boxWidth);
    vline++;
    vline++;
    helpLine(vline, "C", "Clear grid"); vline+=2;
    helpLine(vline, "K", "Clear memo marks"); vline+=2;
    helpLine(vline, "U / R", "Undo / Redo last operation"); vline+=2;
    helpLine(vline, "L", "Load new puzzle"); vline+=2;
    helpLine2(vline, "S", "Save progress","Also saved on Quit"); vline+=2;
    helpLine(vline, "Q", "Quit"); vline+=2;
    
    setColours(BRIGHT_YELLOW, BLACK);
    centreTextInWidth("Press any key",boxHeight - 4, boxWidth);
    wait_for_any_key();
    vdp_reset_viewports();
    vdp_set_text_viewport(0, dims.scrHeightChars, dims.scrWidthChars, 0);
    CLS;
    if (bDrawScreen) drawScreen();
}

void drawNono(int x, int y, int w, int h)
{
    int gap = w / 5;
    int xpos = x;
    drawLetterN(xpos, y, w, h);
    xpos += w+gap;
    drawLetterO(xpos, y, w, h);
    xpos += w+gap;
    drawLetterN(xpos, y, w, h);
    xpos += w+gap;
    drawLetterO(xpos, y, w, h);
}
void openingAnimation()
{
    vdp_mode(129);
    vdp_set_pixel_coordinates();

    for(int s = 4; s < 140; s+= 4)
    {
        int w = s; int h = (s*5)/4;
        int gap = w/5;
        int width = (w * 4) + (gap * 3);
        int xpos = 640/2 - (width/2);
        int ypos = 480/2 - (h/2);

        CLS;
        drawNono(xpos, ypos, w, h);
        vdp_swap();
        delay(100);
    }
    delay(1000);
}

void debugPrep(int X, int Y, int W, int H, int COL)
{
    vdp_gcol(0,0); vdp_filled_rectangle(X*8,Y*8,(X+W)*8,(Y+H)*8);
    vdp_gcol(0,COL); vdp_set_text_colour(COL);
    TAB(X,Y);
}

void clearMemoMarks()
{
    XY it;
    // Find all memo marks, delete adding to undo as we go
    for(it.y=0;it.y<dims.gs;it.y++)
    {
        for(it.x=0;it.x<dims.gs;it.x++)
        {
            if (get_grid(guess, &it) == SQ_MEMOA || get_grid(guess, &it) == SQ_MEMOB)
            {
                doMark(&it, SQ_EMPTY, true);
            }
        }
    }
}
