/*
  vim:ts=4
  vim:sw=4
*/
#ifndef _COMMON_H
#define _COMMON_H

#include "agon/vdp.h"
#include "agon/mos.h"
#include "agon/keyboard.h"
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

#define SQ_EMPTY 0
#define SQ_CROSS 1
#define SQ_FILL 2

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

void init_dims(int gs);
void init_config();
void draw_grid();
void cursorClear(XY* pos);
void cursorDraw(XY* pos);

uint8_t get_grid(uint8_t* grid, XY* pos);
void set_grid(uint8_t* grid, XY* pos, uint8_t val);
void redrawGridSquare(uint8_t* grid, XY* pos);

int input_int(int x, int y, char *msg);
bool input_yn(int x, int y, char *msg);
bool areYouSure();

void title(const char *msg, int bar_col, int title_col);


void wait_clock( clock_t ticks );

// clear keyboard buffer
void clear_keys();
// return after specific key is pressed
uint8_t wait_for_key(uint8_t key);
// return after key is released
uint8_t wait_for_any_key();
// return after key-down event
uint8_t wait_for_any_key_press();

void delay(int timeout);

#endif
