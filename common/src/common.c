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

    if (dims.sclx > 28) {
        dims.sclx = 28;
        dims.scly = 28;
        // recalc width
        dims.gwidth = dims.sclx * gs;
        dims.gheight = dims.scly * gs;
        // recalc offsets
        dims.offy = dims.scrHeightPix - dims.gheight - border;
        dims.offx = dims.scrWidthPix - dims.gwidth - border;
    }
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
    vdp_set_graphics_colour(0, bar_col);
    for (int i=0;i<cl;i++) { putch(27);putch(0x1C); }
    vdp_set_text_colour(title_col);
    vdp_set_graphics_colour(0, title_col);
    printf(" %s ", msg);
    vdp_set_text_colour(bar_col);
    vdp_set_graphics_colour(0, bar_col);
    for (int i=0;i<w - l - cl;i++) { putch(27);putch(0x1C); }

    vdp_set_text_colour(BRIGHT_WHITE);
    vdp_set_graphics_colour(0, BRIGHT_WHITE);
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

void redrawGridSquare(uint8_t* grid, XY* pos)
{
    vdp_move_to(
            dims.offx + (pos->x * dims.sclx) + 1,
            dims.offy + (pos->y * dims.scly) + 1);
    int gap = (dims.sclx * 4) / 5;
    int gap2 = gap*2;
    uint8_t g = get_grid(grid, pos);
    switch (g) {
        case SQ_EMPTY:
            vdp_gcol(0, config.col_gridbg);
            vdp_plot(PLOT_TYPE_RECT_FILL|PLOT_MODE_FG_REL, dims.sclx-2, dims.scly-2);
            break;
        case SQ_CROSS:
            // clear
            vdp_gcol(0, config.col_gridbg);
            vdp_plot(PLOT_TYPE_RECT_FILL|PLOT_MODE_FG_REL, dims.sclx-2, dims.scly-2);
            // move to sart of cross top-left
            vdp_move_to(
                    dims.offx + (pos->x * dims.sclx) + gap,
                    dims.offy + (pos->y * dims.scly) + gap);
            vdp_gcol(0, config.col_mark);
            vdp_plot(PLOT_TYPE_SOLID_ALL|PLOT_MODE_FG_REL, dims.sclx-gap2, dims.scly-gap2);
            vdp_plot(PLOT_TYPE_SOLID_ALL|PLOT_MODE_MOVE_REL, 0, 0-(dims.scly-gap2));
            vdp_plot(PLOT_TYPE_SOLID_ALL|PLOT_MODE_FG_REL, 0-(dims.sclx-gap2), dims.scly-gap2);
            break;
        case SQ_FILL:
            vdp_gcol(0, config.col_mark);
            vdp_plot(PLOT_TYPE_RECT_FILL|PLOT_MODE_FG_REL, dims.sclx-2, dims.scly-2);
            break;
    }
}

// Access functions to grid arrays
void set_grid(uint8_t* grid, XY* pos, uint8_t val)
{
    grid[pos->x + (pos->y * dims.gs)] = val;
}
uint8_t get_grid(uint8_t* grid, XY* pos)
{
    return grid[pos->x + (pos->y * dims.gs)];
}


// Calculate column runs and display
bool calc_column_run(uint8_t* grid, int col)
{
    XY p;
    p.x = col;
    
    // set where to draw numbers and set mode to draw text at graph cursor
    int x = dims.offx + (col * dims.sclx) + (dims.sclx - 8)/2; 
    int y = dims.offy - 10;
    vdp_write_at_graphics_cursor();

    // reset column
    vdp_set_graphics_colour(0, BLACK);
    vdp_filled_rectangle(
            dims.offx + (col * dims.sclx),
            8,
            dims.offx + (col * dims.sclx) + dims.sclx,
            dims.offy - 2);
    vdp_set_graphics_colour(0, BRIGHT_WHITE);

    // check all columns are set
    for (p.y=0;p.y<dims.gs;p.y++) {
        if (get_grid(grid, &p)==SQ_EMPTY) return false;
    }

    // start from bottom and count
    int run = 0;
    for (p.y=dims.gs-1; p.y>=0; p.y--) {
        if (get_grid(grid, &p)==SQ_FILL) {
            run++;
            continue;
        }
        if (get_grid(grid, &p)==SQ_CROSS && run>0) {
            if (run>9) {
                vdp_set_graphics_colour(0, BRIGHT_YELLOW);
                vdp_move_to(x-2, y);
                printf("1");
                vdp_move_to(x+3, y);
                printf("%d",run%10);
            } else {
                vdp_set_graphics_colour(0, BRIGHT_WHITE);
                vdp_move_to(x, y);
                printf("%d",run);
            }
            run = 0;
            y -= 8;
        }
    }
    if (run > 0) {
        if (run>9) {
            vdp_set_graphics_colour(0, BRIGHT_YELLOW);
                vdp_move_to(x-2, y);
                printf("1");
                vdp_move_to(x+2, y);
                printf("%d",run%10);
        } else {
            vdp_set_graphics_colour(0, BRIGHT_WHITE);
            vdp_move_to(x, y);
            printf("%d",run);
        }
    }
    return true;
}

// Calculate row runs and display
bool calc_row_run(uint8_t* grid, int row)
{
    XY p;
    p.y = row;
    
    // set where to draw numbers and set mode to draw text at graph cursor
    int x = dims.offx - 10;
    int y = dims.offy + (row * dims.scly) + (dims.scly - 8)/2; 
    vdp_write_at_graphics_cursor();

    // reset row
    vdp_set_graphics_colour(0, BLACK);
    vdp_filled_rectangle(
            8,
            dims.offy + (row * dims.scly),
            dims.offx - 2,
            dims.offy + (row * dims.scly) + dims.scly);

    vdp_set_graphics_colour(0, BRIGHT_WHITE);
    vdp_set_graphics_colour(0, BRIGHT_WHITE);

    // check all rows are set
    for (p.x=0; p.x<dims.gs; p.x++) {
        if (get_grid(grid, &p)==SQ_EMPTY) return false;
    }

    // start from right and count back
    int run = 0;
    int first = true;
    for (p.x=dims.gs-1; p.x>=0; p.x--) {
        if (get_grid(grid, &p)==SQ_FILL) {
            run++;
            continue;
        }
        if (get_grid(grid, &p)==SQ_CROSS && run>0) {
            if (run>9) {
                vdp_set_graphics_colour(0, BRIGHT_YELLOW);
                vdp_move_to(x-8, y);
            } else {
                vdp_set_graphics_colour(0, BRIGHT_WHITE);
                vdp_move_to(x, y);
            }
            if (first) {
                printf("%d",run);
                x -= 16; if (run>9) x -= 8;
            } else {
                x -= 8;
                printf("%d,",run);
                x -= 8; if (run>9) x -= 8;
            }
            run = 0;
            first = false;
        }
    }
    if (run > 0) {
        if (run>9) {
            vdp_set_graphics_colour(0, BRIGHT_YELLOW);
            vdp_move_to(x-8, y);
        } else {
            vdp_set_graphics_colour(0, BRIGHT_WHITE);
            vdp_move_to(x, y);
        }
        if (first) {
            printf("%d",run);
            x -= 16; if (run>9) x -= 8;
        } else {
            x -= 8;
            printf("%d,",run);
            x -= 8; if (run>9) x -= 8;
        }
    }
    return true;
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
// return after specific key is released
uint8_t wait_for_key_up(uint8_t key)
{
    struct keyboard_event_t e;
    do {
        while (!kbuf_poll_event(&e)) {}
    } while (e.vkey != key && e.isdown);
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

    return e.vkey;
}

void wait_clock( clock_t ticks )
{
    clock_t ticks_now = clock();

    do {
    } while ( clock() - ticks_now < ticks );
}

int input_int(int x, int y, const char *msg)
{
	int num;
	TAB(x,y);
	printf("%s:",msg);
	scanf("%d",&num);
	return num;
}
bool input_yn(int x, int y, const char *msg)
{
	bool yn = false;
    char str[12];
	TAB(x,y);
	printf("%s:",msg);
	fgets(&str[0], 12, stdin);
    if (str[0] == 'y' || str[0] == 'Y') yn = true;
	return yn;
}

bool areYouSure(const char *msg)
{
    // TODO make this a popup box

    vdp_write_at_text_cursor();
    vdp_set_text_colour(BRIGHT_WHITE);
    TAB(0,2);
    for (int i=0; i< strlen(msg)+8; i++) { printf(" "); }

    bool choice = input_yn(0,2, msg);

    TAB(0,2);
    for (int i=0; i< strlen(msg)+8; i++) { printf(" "); }

    return choice;
}

bool checkFilename(char *fname)
{
    return true;
}
bool saveBoard(uint8_t *grid, char* fname) 
{
    FILE* fptr = fopen(fname, "wb");
    if (!fptr) return false;
    fputc(dims.gs, fptr);
    for (int i=0; i<dims.gs*dims.gs; i++) { fputc(grid[i], fptr); }
    fclose(fptr);
    return true;
}
bool loadBoard(uint8_t *grid, char* fname) 
{
    FILE* fptr = fopen(fname, "rb");
    if (!fptr) {
        printf("Cannot open %s\n", fname);
        return false;
    }
    int GS = fgetc(fptr);
    if (GS != dims.gs) {
        printf("Incorrect size data\n");
        fclose(fptr);
        return false;
    }
    for (int i=0; i<dims.gs*dims.gs; i++) { grid[i] = fgetc(fptr); }
    fclose(fptr);

    return true;
}

bool isGridComplete(uint8_t* grid)
{
    for (int i=0; i<dims.gs*dims.gs; i++) {
        if (grid[i] == SQ_EMPTY) return false;
    }
    return true;
}

void refreshBoard(uint8_t* grid)
{
    XY pos;
    for (pos.y=0;pos.y<dims.gs;pos.y++) {
        for (pos.x=0;pos.x<dims.gs;pos.x++) {
            redrawGridSquare(grid, &pos);
        }
        calc_row_run(grid, pos.y);
    }
    for (pos.x=0;pos.x<dims.gs;pos.x++) {
        calc_column_run(grid, pos.x);
    }
}

void msgBox(int width, int height, char *msg)
{
    // centre
    int x = (dims.scrWidthChars - width) / 2;
    int y = (dims.scrHeightChars - height) / 2;
    // clear a part of the screen
    vdp_set_graphics_colour(0, YELLOW);
    vdp_set_text_colour(BRIGHT_WHITE);
    vdp_filled_rectangle(
            x*8, y*8,
            (x+width)*8-1, (y+height+1)*8-1);
    // draw box
    vdp_set_graphics_colour(0, BRIGHT_WHITE);
    vdp_set_text_colour(BRIGHT_WHITE);
    TAB(x,y);
    for (int i=0;i<width;i++) { putch(27);putch(0x1C); }
    for (int i=1;i<height;i++) {
        TAB(x,y+i);putch(27);putch(0x1C);
        TAB(x+width-1,y+i);putch(27);putch(0x1C);
    }
    TAB(x,y+height);
    for (int i=0;i<width;i++) { putch(27);putch(0x1C); }

    vdp_set_text_colour(BRIGHT_WHITE);
    vdp_set_text_colour(128+BLACK);
    vdp_set_text_bg_colour(BLACK);
    vdp_set_graphics_bg_colour(0, BLACK);
    TAB(x+2, y+height/2); printf("%s",msg);
}
