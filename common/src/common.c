/*
  vim:ts=4
  vim:sw=4
*/
#include "common.h"

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

extern uint8_t* grid;
extern CONFIG config;
extern DIMS dims;

void init_dims(int gs)
{
    int border = 10;
    dims.gs = gs;
    // read the screen dimensions
    dims.scrWidthPix = getsysvar_scrwidth();
    dims.scrHeightPix = getsysvar_scrheight();
    dims.scrWidthChars = getsysvar_scrCols();
    dims.scrHeightChars = getsysvar_scrRows();

    // must leave space for enough 8x8 numbers above grid
    int max_nums = ((int) (gs/2)) + 1;

    dims.gheight = dims.scrHeightPix - (8 * max_nums) - border; 

    // if grid height is > 3/4 of the screen, scale it back
    if (dims.gheight > (border + (dims.scrHeightPix * 3) / 4)) {
        dims.gheight = border + ((dims.scrHeightPix * 3) / 4);
    }

    // make it an integer multiple of grid size
    dims.gheight = ((int) dims.gheight / gs)  * gs;

    // easier to make height=width
    dims.gwidth = dims.gheight;// * dims.scrWidthPix / dims.scrHeightPix;

    // finally get offset of grid
    dims.offy = dims.scrHeightPix - dims.gheight - border;
    dims.offx = dims.scrWidthPix - dims.gwidth - border;

    // and scale
    dims.sclx = dims.gwidth / gs;
    dims.scly = dims.gheight / gs;
}

void init_config()
{
    config.col_gridbg = 15;
    config.col_gridmin = 7;
    config.col_gridmaj = 4;
    config.col_mark = 0;
    config.col_cross = 8;
    config.col_cursor = 9;
}

void draw_grid() 
{
    // background of whole grid as rect
    vdp_gcol(0, config.col_gridbg);
    vdp_filled_rectangle(dims.offx, dims.offy, dims.offx + dims.gwidth, dims.offy + dims.gheight);

    // Horizontal grid lines
    for (int i=0; i <= dims.gs; i++)
    {
        int y = dims.offy + (i * dims.scly);
        if ((i % 5) == 0) vdp_gcol(0, config.col_gridmaj); else vdp_gcol(0, config.col_gridmin);
        vdp_line(dims.offx, y, dims.offx + dims.gwidth, y);
    }
    // Vertical grid lines
    for (int i=0; i <= dims.gs; i++)
    {
        int x = dims.offx + (i * dims.sclx);
        if ((i % 5) == 0) vdp_gcol(0, config.col_gridmaj); else vdp_gcol(0, config.col_gridmin);
        vdp_line(x, dims.offy, x, dims.offy + dims.gheight);
    }
}


void title(const char *msg, int bar_col, int title_col)
{
    TAB(0,0);
    int w = getsysvar_scrCols();
    int l = strlen(msg) + 2; int cl = (w - l) / 2;
    vdp_set_text_colour(bar_col);
    for (int i=0;i<cl;i++) { putch(27);putch(0x1C); }
    vdp_set_text_colour(title_col);
    printf(" %s ", msg);
    vdp_set_text_colour(bar_col);
    for (int i=0;i<w - l - cl;i++) { putch(27);putch(0x1C); }

    vdp_set_text_colour(15);
}

void cursorClear(XY* pos)
{
    vdp_move_to(
            dims.offx + (pos->x * dims.sclx),
            dims.offy + (pos->y * dims.scly));
    if (pos->y % 5 == 0) vdp_gcol(0, config.col_gridmaj); else vdp_gcol(0, config.col_gridmin);
    vdp_plot(PLOT_TYPE_SOLID_ALL|PLOT_MODE_FG_REL, dims.sclx, 0);
    if (pos->x % 5 == 4) vdp_gcol(0, config.col_gridmaj); else vdp_gcol(0, config.col_gridmin);
    vdp_plot(PLOT_TYPE_SOLID_ALL|PLOT_MODE_FG_REL, 0, dims.scly);
    if (pos->y % 5 == 4) vdp_gcol(0, config.col_gridmaj); else vdp_gcol(0, config.col_gridmin);
    vdp_plot(PLOT_TYPE_SOLID_ALL|PLOT_MODE_FG_REL, -dims.sclx, 0);
    if (pos->x % 5 == 0) vdp_gcol(0, config.col_gridmaj); else vdp_gcol(0, config.col_gridmin);
    vdp_plot(PLOT_TYPE_SOLID_ALL|PLOT_MODE_FG_REL, 0, -dims.scly);
}

void cursorDraw(XY* cursor)
{
    vdp_gcol(0, config.col_cursor);
    vdp_move_to(dims.offx + cursor->x * dims.sclx, dims.offy + cursor->y * dims.scly);
    vdp_plot(PLOT_TYPE_SOLID_ALL|PLOT_MODE_FG_REL, dims.sclx, 0);
    vdp_plot(PLOT_TYPE_SOLID_ALL|PLOT_MODE_FG_REL, 0, dims.scly);
    vdp_plot(PLOT_TYPE_SOLID_ALL|PLOT_MODE_FG_REL, -dims.sclx, 0);
    vdp_plot(PLOT_TYPE_SOLID_ALL|PLOT_MODE_FG_REL, 0, -dims.scly);
}

void redrawGridSquare(XY* pos)
{
    vdp_move_to(
            dims.offx + (pos->x * dims.sclx) + 1,
            dims.offy + (pos->y * dims.scly) + 1);
    uint8_t g = get_grid(pos);
    switch (g) {
        case SQ_EMPTY:
            vdp_gcol(0, config.col_gridbg);
            vdp_plot(PLOT_TYPE_RECT_FILL|PLOT_MODE_FG_REL, dims.sclx-2, dims.scly-2);
            break;
        case SQ_CROSS:
            vdp_gcol(0, config.col_mark);
            vdp_plot(PLOT_TYPE_SOLID_ALL|PLOT_MODE_FG_REL, dims.sclx-2, dims.scly-2);
            vdp_plot(PLOT_TYPE_SOLID_ALL|PLOT_MODE_MOVE_REL, 0, 0-(dims.scly-2));
            vdp_plot(PLOT_TYPE_SOLID_ALL|PLOT_MODE_FG_REL, 0-(dims.sclx-2), dims.scly-2);
            break;
        case SQ_FILL:
            vdp_gcol(0, config.col_mark);
            vdp_plot(PLOT_TYPE_RECT_FILL|PLOT_MODE_FG_REL, dims.sclx-2, dims.scly-2);
            break;
    }
}

void set_grid(XY* pos, uint8_t val)
{
    grid[pos->x + (pos->y * dims.gs)] = val;
}
uint8_t get_grid(XY* pos)
{
    return grid[pos->x + (pos->y * dims.gs)];
}

// clear keyboard buffer
void clear_keys()
{
    struct keyboard_event_t e;
    while (kbuf_poll_event(&e)) {}
}
// return after specific key is pressed
uint8_t wait_for_key(uint8_t key)
{
    struct keyboard_event_t e;
    do {
        while (!kbuf_poll_event(&e)) {}
    } while (e.vkey != key);
    return key;
}
// return after key is released
uint8_t wait_for_any_key()
{
    struct keyboard_event_t e;
    int gotkey = 0;
    do {
        int ret = kbuf_poll_event(&e);
        if (ret && !e.isdown) gotkey = 1;
    } while ( gotkey == 0 );

    return e.vkey;
}
// return after key-down event
uint8_t wait_for_any_key_press()
{
    struct keyboard_event_t e;
    int gotkey = 0;
    do {
        int ret = kbuf_poll_event(&e);
        if (ret && e.isdown) gotkey = 1;
    } while ( gotkey == 0 );
}

void wait_clock( clock_t ticks )
{
    clock_t ticks_now = clock();

    do {
    } while ( clock() - ticks_now < ticks );
}
