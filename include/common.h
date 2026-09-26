/*
  vim:ts=4
  vim:sw=4
*/
#ifndef _COMMON_H
#define _COMMON_H

#include "agon/vdp.h"
#include "agon/mos.h"
#include "agon/keyboard.h"
#include "agon/timer.h"
#include "keydefines.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define COL(C) vdp_set_text_colour(C)
#define TAB(X,Y) vdp_cursor_tab(X,Y)
#define CLS vdp_cls()

#define PLOT_MODE_MOVE_REL 0
#define PLOT_MODE_FG_REL 1
#define PLOT_MODE_INV_REL 2
#define PLOT_MODE_BG_REL 3
#define PLOT_MODE_MOVE_ABS 4
#define PLOT_MODE_FG_ABS 5
#define PLOT_MODE_INV_ABS 6
#define PLOT_MODE_BG_ABS 7

#define PLOT_TYPE_SOLID_ALL          0x00
#define PLOT_TYPE_SOLID_NOFINAL      0x08
#define PLOT_TYPE_DOT_DASH_ALL       0x10
#define PLOT_TYPE_DOT_DASH_NOFINAL   0x18
#define PLOT_TYPE_SOLID_NOSTART      0x20
#define PLOT_TYPE_SOLID_NOENDS       0x28
#define PLOT_TYPE_DOT_DASH_ALL_CONT       0x30
#define PLOT_TYPE_DOT_DASH_NOFINAL_CONT   0x38
#define PLOT_TYPE_POINT              0x40
#define PLOT_TYPE_LR_LINEFILL        0x48
#define PLOT_TYPE_TRIANGLE_FILL      0x50
#define PLOT_TYPE_R_LINEFILL         0x58
#define PLOT_TYPE_RECT_FILL          0x60
#define PLOT_TYPE_LR_FILL_FG         0x68
#define PLOT_TYPE_PARA_FILL          0x70
#define PLOT_TYPE_LR_FILL_NONFG      0x78
#define PLOT_TYPE_FLOOD_FILL_NONBG   0x80
#define PLOT_TYPE_FLOOD_FILL_FG      0x88
#define PLOT_TYPE_CIRCLE_OUTLINE     0x90
#define PLOT_TYPE_CIRCLE_FILL        0x98
#define PLOT_TYPE_CIRCULAR_ARC       0xA0
#define PLOT_TYPE_CIRCULAR_SEGMENT   0xA8
#define PLOT_TYPE_CIRCULAR_SECTOR    0xB0
#define PLOT_TYPE_RECT_COPY_MOVE     0xB8
#define PLOT_TYPE_ELIPSE_OUTLINE     0xC0
#define PLOT_TYPE_ELIPSE_FILL        0xC8
#define PLOT_TYPE_FILL_PATH          0xD8

#define SQ_EMPTY 0
#define SQ_FILL  1
#define SQ_CROSS 2

typedef struct {
    int gs;
    int scrWidthPix;    // Screen width in pixels
    int scrHeightPix;    // Screen height in pixels
    int scrWidthChars;    // Screen width in chars
    int scrHeightChars;    // Screen height in chars
    int offx;   // offset at which to draw grid
    int offy;
    int sclx;   // Scale of grid squares
    int scly;
    int gwidth;     // calculated grid width
    int gheight;    // calculated grid height
    int mscl;       // mini grid scale(pixels)
    int moffx;      // mini grid offset x
    int moffy;      // mini grid offset y
} DIMS;

typedef struct {
    int col_gridbg;     // Grid background colour
    int col_gridmin;    // Grid minor lines colour
    int col_gridmaj;    // Grid major colour
    int col_mark;       // Mark colour
    int col_cross;      // Cross colour
    int col_cursor;     // Cursor colour
} CONFIG;

typedef struct {
    int x;
    int y;
} XY; 

struct ULENTRY_T {
    uint8_t x;
    uint8_t y;
    uint8_t old_state;
    uint8_t new_state;
    struct ULENTRY_T *next; 
    struct ULENTRY_T *prev; 
};

typedef struct ULENTRY_T ULENTRY;


void init_dims(int gs);
void init_config();
void draw_grid();
void cursorClear(XY* pos);
void cursorDraw(XY* pos);

uint8_t get_grid(uint8_t* grid, XY* pos);
void set_grid(uint8_t* grid, XY* pos, uint8_t val);
void redrawGridSquare(uint8_t* grid, XY* pos);

bool calc_column_run(uint8_t* grid, int col);
bool calc_row_run(uint8_t* grid, int row);

void processString(char *str, char *str2);
int input_int(int x, int y, const char *msg);
bool input_yn(int x, int y, const char *msg);
bool areYouSure(const char *msg);

void title(const char *msg, int bar_col, int title_col);
void centreText(char *msg, int y);
void centreTextInWidth(char *msg, int y, int width);
void setColours(int fg, int bg);


void wait_clock( clock_t ticks );

// clear keyboard buffer
void clear_keys();
// return after specific key is pressed
uint8_t wait_for_key(uint8_t key);
// return after specific key is released
uint8_t wait_for_key_up(uint8_t key);
// return after key is released
uint8_t wait_for_any_key();
// return after key-down event
uint8_t wait_for_any_key_press();

bool checkFilename(char *fname);
bool saveBoard(uint8_t *grid, char* fname);
bool loadBoard(uint8_t *grid, char* fname);
bool isGridComplete(uint8_t* grid);
bool checkSolution(uint8_t* guess, uint8_t* solution);

void refreshBoard(uint8_t* grid);
void refreshCounts(uint8_t* grid);

void msgBoxModal(int width_chars, int height_chars, int border_col, int text_col);

void viewMini(uint8_t* grid);

void drawLetterN(int left,int right,int width,int height);
void drawLetterO(int left,int right,int width,int height);

#endif
