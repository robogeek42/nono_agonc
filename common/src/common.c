/*
  vim:ts=4
  vim:sw=4
*/
#include "common.h"

void init_dims(struct dims_t* d, int gs)
{
    int border = 10;
    d->gs = gs;
    // read the screen dimensions
    d->scrWidthPix = getsysvar_scrwidth();
    d->scrHeightPix = getsysvar_scrheight();
    d->scrWidthChars = getsysvar_scrCols();
    d->scrHeightChars = getsysvar_scrRows();

    // must leave space for enough 8x8 numbers above grid
    int max_nums = ((int) (gs/2)) + 1;

    d->gheight = d->scrHeightPix - (8 * max_nums) - border; 

    // if grid height is > 3/4 of the screen, scale it back
    if (d->gheight > (border + (d->scrHeightPix * 3) / 4)) {
        d->gheight = border + ((d->scrHeightPix * 3) / 4);
    }

    // make it an integer multiple of grid size
    d->gheight = ((int) d->gheight / gs)  * gs;

    // easier to make height=width
    d->gwidth = d->gheight;// * d->scrWidthPix / d->scrHeightPix;

    // finally get offset of grid
    d->offy = d->scrHeightPix - d->gheight - border;
    d->offx = d->scrWidthPix - d->gwidth - border;

    // and scale
    d->sclx = d->gwidth / gs;
    d->scly = d->gheight / gs;
}

void init_config(struct config_t* c)
{
    c->col_gridbg = 15;
    c->col_gridmin = 7;
    c->col_gridmaj = 8;
    c->col_mark = 0;
    c->col_cross = 8;
    c->col_cursor = 9;
}

void draw_grid(struct dims_t* d, struct config_t* c) 
{
    // background of whole grid as rect
    vdp_gcol(0, c->col_gridbg);
    vdp_filled_rectangle(d->offx, d->offy, d->offx + d->gwidth, d->offy + d->gheight);

    // Horizontal grid lines
    for (int i=0; i <= d->gs; i++)
    {
        int y = d->offy + (i * d->scly);
        if ((i % 5) == 0) vdp_gcol(0, c->col_gridmaj); else vdp_gcol(0, c->col_gridmin);
        vdp_line(d->offx, y, d->offx + d->gwidth, y);
    }
    // Vertical grid lines
    for (int i=0; i <= d->gs; i++)
    {
        int x = d->offx + (i * d->sclx);
        if ((i % 5) == 0) vdp_gcol(0, c->col_gridmaj); else vdp_gcol(0, c->col_gridmin);
        vdp_line(x, d->offy, x, d->offy + d->gheight);
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

void clear_keys()
{
    struct keyboard_event_t e;
    while (kbuf_poll_event(&e)) {}
}
uint8_t wait_for_key(uint8_t key)
{
    struct keyboard_event_t e;
    do {
        while (!kbuf_poll_event(&e)) {}
    } while (e.vkey != key);
    return key;
}
uint8_t wait_for_any_key()
{
    struct keyboard_event_t e;
    while (!kbuf_poll_event(&e)) {}
    return e.vkey;
}
uint8_t wait_for_any_key_press()
{
    struct keyboard_event_t e;
    while (!kbuf_poll_event(&e)) {}
    return e.vkey;
}

void wait_clock( clock_t ticks )
{
    clock_t ticks_now = clock();

    do {
    } while ( clock() - ticks_now < ticks );
}
