#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include <sdl.h>

#include "utils.h"

//
// defines
//

#define MAX_PAGE 7

// xxx check these
#define ROW2Y(r) ((r) * sdl_char_height)  // xxx ctr vs ...
#define ROW2Y_CTR(r) ((r) * sdl_char_height + sdl_char_height/2)

#define NK2X(n,k) ((sdl_win_width/2/(n)) + (k) * (sdl_win_width/(n)))

#define EVID_PREV_PAGE   1
#define EVID_NEXT_PAGE   2
#define EVID_END_PROGRAM 3

//
// variables
//

static sdl_event_t event;

//
// prototypes
//

static void page_3_cleanup(void);
static void page_5_cleanup(void);
static void render_page(int n, bool init);
static void render_page_0(bool init);
static void render_page_1(bool init);
static void render_page_2(bool init);
static void render_page_3(bool init);
static void render_page_4(bool init);
static void render_page_5(bool init);
static void render_page_6(bool init);

// -----------------  MAIN  ------------------------------------------

int main(int argc, char **argv)
{
    int  i;
    int  pagenum = 5;
    bool end_program = false;
    bool is_ez_app = (argc > 0 && strcmp(argv[0], "ez_app") == 0);
    bool init = true;

    // print args 
    printf("argc = %d\n", argc);
    for (i = 0; i < argc; i++) {
        printf("argv[%d] = '%s'\n", i, argv[i]);
    }
    printf("is_ez_app = %d\n", is_ez_app);

    // if not ez_app then call sdl_init
    if (!is_ez_app && sdl_init() != 0) {
        printf("ERROR: sdl_init failed\n");
        return 1;
    }

    // get window and char sized, these are global variables from sdl.c
    printf("sdl_win_width/height  = %d %d\n", sdl_win_width, sdl_win_height);
    printf("sdl_char_width/height = %d %d\n", sdl_char_width, sdl_char_height);

    // test calling a routine that is defined in another file
    utils_proc();

    // loop, displaying the currently selected pagenum and proces events
    while (!end_program) {
        // init the backbuffer
        sdl_display_init(COLOR_BLACK);

        // XXX
        // xxx register for swipe and motion

        // render to the backbuffer
        render_page(pagenum, init);
        init = false;

        // present the display
        sdl_display_present();

        // wait for an event with 100 ms timeout;
        // if no event then redraw display
        sdl_get_event(100000, &event);
        if (event.event_id == -1) {
            continue;
        }

        // process event
        // note that EVID_QUIT is always provided xxx?
        switch (event.event_id) {
        case EVID_QUIT:
        case EVID_END_PROGRAM:
            end_program = true;
            break;
        case EVID_SWIPE_RIGHT:
        case EVID_PREV_PAGE:
            if (--pagenum < 0) {
                pagenum = MAX_PAGE-1;
            }
            init = true;
            break;
        case EVID_SWIPE_LEFT:
        case EVID_NEXT_PAGE:
            if (++pagenum >= MAX_PAGE) {
                pagenum = 0;
            }
            init = true;
        case EVID_MOTION:
            // motion events are handled in the pagges that
            // utilize motion events; 'event' is a global variable
            break;
        }

        // if end_program flag is set then break
        if (end_program) {
            break;
        }
    }

    // call cleanup routines, to free allocations
    page_3_cleanup();
    page_5_cleanup();

    // if not ez_app then call sdl_exit
    if (!is_ez_app) {
        sdl_exit();
    }

    // return success
    return 0;
}

// -----------------  RENDER PAGS PROC  -----------------------

// picoc: picoc does not support this being static, causes crash
char *title[] = {       // Page
        "Unit Test",    //   0
        "Font",         //   1
        "Sizeof",       //   2
        "Multi Lines",  //   3
        "Drawing",      //   4
        "Textures",     //   5
        "Colors",       //   6
            };

static void render_page(int pagenum, bool init)
{
    sdl_loc_t *loc;

    // render text and register events for the following:
    // "<" - previous page
    // ">" - next page
    // 'X' - end prorgram
    sdl_print_init(10, COLOR_WHITE, COLOR_BLACK);
    loc = sdl_render_printf_xyctr(NK2X(3,0), sdl_win_height-sdl_char_height/2, "%s", "<");
    sdl_register_event(loc, EVID_PREV_PAGE);
    loc = sdl_render_printf_xyctr(NK2X(3,1), sdl_win_height-sdl_char_height/2, "%s", ">");
    sdl_register_event(loc, EVID_NEXT_PAGE);
    loc = sdl_render_printf_xyctr(NK2X(3,2), sdl_win_height-sdl_char_height/2, "%s", "X");
    sdl_register_event(loc, EVID_END_PROGRAM);

    // display title line 
    sdl_print_init(20, COLOR_WHITE, COLOR_BLACK);
    sdl_render_text_xyctr(NK2X(1,0), ROW2Y_CTR(0), title[pagenum]);

    // render the page
    switch (pagenum) {
    case 0: render_page_0(init); break;
    case 1: render_page_1(init); break;
    case 2: render_page_2(init); break;
    case 3: render_page_3(init); break;
    case 4: render_page_4(init); break;
    case 5: render_page_5(init); break;
    case 6: render_page_6(init); break;
    }
}

// -----------------  PAGE 0: CLOCK  --------------------------

static void render_page_0(bool init)
{
    time_t t;
    struct tm *tm;
    char str[100];

    time(&t);
    tm = localtime(&t);

    sprintf(str, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
    sdl_render_printf_xyctr(sdl_win_width/2, sdl_win_height/3, "%s", str);
}

// -----------------  PAGE 1: FONT  ---------------------------

static void render_page_1(bool init)
{
    int i, ch=0;
    char str[32];

    for (i = 0; i < 16; i++) {
        sprintf(str, "%02x %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
                i*16,
                ch+0, ch+1, ch+2, ch+3, ch+4, ch+5, ch+6, ch+7,
                ch+8, ch+9, ch+10, ch+11, ch+12, ch+13, ch+14, ch+15);

        sdl_render_text(0, ROW2Y(i+2), str);
        ch += 16;
    }
}

// -----------------  PAGE 2: SIZEOF  -------------------------

static void render_page_2(bool init)
{
    int r = 2;

    sdl_render_printf(0, ROW2Y(r++), "sizoef(char)   = %zd", sizeof(char));
    sdl_render_printf(0, ROW2Y(r++), "sizoef(short)  = %zd", sizeof(short));
    sdl_render_printf(0, ROW2Y(r++), "sizoef(int)    = %zd", sizeof(int));
    sdl_render_printf(0, ROW2Y(r++), "sizoef(long)   = %zd", sizeof(long));
    sdl_render_printf(0, ROW2Y(r++), "sizoef(size_t) = %zd", sizeof(size_t));
    sdl_render_printf(0, ROW2Y(r++), "sizoef(off_t)  = %zd", sizeof(off_t));
    sdl_render_printf(0, ROW2Y(r++), "sizoef(time_t) = %zd", sizeof(time_t));
    sdl_render_printf(0, ROW2Y(r++), "sizeof(1)      = %zd", sizeof(123));
    sdl_render_printf(0, ROW2Y(r++), "sizeof(1ULL);  = %zd", sizeof(123UL));
}

// -----------------  PAGE 3: MULTI LINE TEXT  ----------------

// This tests both
// - sdl_render_multiline_text, and
// - sdl_render_multiline_text_2
// on alternate entering of this page.

static int y_top;
static int y_display_begin;
static int y_display_end;
static char lines[2000];
static char *lines_2[100];
static bool test_v1;
static bool first_call = true;

static void render_page_3(bool init)
{
    if (first_call) {
        char *p = lines;
        for (int i = 0; i < 100; i++) {
            p += sprintf(p, "Line %d\n", i);
        }

        for (int i = 0; i < 100; i++) {
            lines_2[i] = malloc(20);
            sprintf(lines_2[i], "Line-V2 %d", i);
        }

        first_call = false;
    }

    if (init) {
        y_top = ROW2Y(2); 
        y_display_begin = ROW2Y(2);
        y_display_end = sdl_win_height-3*sdl_char_height;  // xxx improve?
        test_v1 = !test_v1;
    }

    if (test_v1) {
        sdl_render_multiline_text(y_top, y_display_begin, y_display_end, lines);
    } else {
        sdl_render_multiline_text_2(y_top, y_display_begin, y_display_end, lines_2, 100);
    }

    if (event.event_id == EVID_MOTION) {
        y_top += event.u.motion.yrel;
        if (y_top >= y_display_begin) {
            y_top = y_display_begin;
        }
    }
}

static void page_3_cleanup(void)
{
    for (int i = 0; i < 100; i++) {
        free(lines_2[i]);
    }
}

// -----------------  PAGE 4: DRAWING  ------------------------

static void add_point(sdl_point_t **p, int x, int y);

static void render_page_4(bool init)
{
    // draw rect around perimeter
    sdl_render_rect(0, 0, sdl_win_width, sdl_win_height, 2, COLOR_PURPLE);

    // draw fill rect, y = 170 .. 400
    sdl_render_fill_rect(100, 170, 800, 230, COLOR_RED);

    // draw circles, y = 400 .. 500
    sdl_render_circle(1*sdl_win_width/4, 450, 50, 3, COLOR_YELLOW);
    sdl_render_circle(2*sdl_win_width/4, 450, 50, 3, COLOR_YELLOW);
    sdl_render_circle(3*sdl_win_width/4, 450, 50, 3, COLOR_YELLOW);

    // draw 6 lines, y = 500 .. 600
    for (int y = 500; y <= 600; y += 20) {
        sdl_render_line(0, y, 1000, y, COLOR_WHITE);
    }

    // draw 3 lines to make a triangle, y = 600 .. 800
    sdl_point_t pts[4], *ptsx=pts;
    add_point(&ptsx, 500, 600);
    add_point(&ptsx, 700, 800);
    add_point(&ptsx, 300, 800);
    add_point(&ptsx, 500, 600);
    sdl_render_lines(pts, 4, COLOR_RED);

    // draw 2 squares and vary intensity and wavelen, y = 800 .. 900
    static double inten;
    int color;
    inten = inten + 0.01;
    if (inten > 1) inten = 0;
    color = sdl_scale_color(COLOR_YELLOW, inten);
    sdl_render_fill_rect(100, 800, 100, 100, color);

    static double wavelen = 750;
    wavelen -= 2;
    if (wavelen < 440) wavelen = 750;
    color = sdl_wavelength_to_color(wavelen);
    sdl_render_fill_rect(800, 800, 100, 100, color);

    // draw points with varying size, y = 1000
    color = sdl_create_color(0, 255, 0, 255);
    for (int pointsize = 0; pointsize <= 9; pointsize++) {
        sdl_render_point(pointsize*100+50, 1000, color, pointsize);
    }

    // draw 10 points of the same size, y = 1100
    sdl_point_t points[10];
    for (int i = 0; i < 10; i++) {
        points[i].x = i*100+50;
        points[i].y = 1100;
    }
    sdl_render_points(points, 10, COLOR_PURPLE, 5);
}

static void add_point(sdl_point_t **p, int x, int y)
{
    (*p)->x = x;
    (*p)->y = y;
    (*p)++;
}

// -----------------  PAGE 5: TEXTURES  -----------------------

static sdl_texture_t *circle;
static sdl_texture_t *text;

static void render_page_5(bool init)
{
    int w, h, fd;
    sdl_texture_t *t;
    sdl_pixels_t *pixels;

    // if the circle texture has not been initialized then
    // init the textures used by this test:
    // - circle: sdl_create_filled_circle_texture
    // - text:   sdl_create_text_texture
    if (circle == NULL) {
        circle = sdl_create_filled_circle_texture(100, COLOR_RED);
        text   = sdl_create_text_texture("XXXXX");
    }

    // xxx fixup the following ...

    // render the circle texture at varying x location, y = 200 .. 400
    static int circle_x=-200;
    sdl_render_texture(circle_x, 200, -1, -1, 0, circle);
    circle_x += 10;
    if (circle_x > 1000) circle_x = -200;

    // render the circle texture using scaling, y = 400 .. 600
    sdl_render_texture(500-200, 400, 400, 200, 0, circle);

    // render text texture, at y = 500
    sdl_query_texture(text, &w, &h);
    sdl_render_texture(0, 500-h/2, -1, -1, 0, text);

    // rotate and render the text texture at y = 600 .. 850
    static double angle = 0;
    angle += 5;
    sdl_render_texture(500-w/2, 600+w/2-h/2, -1, -1, angle, text);

    // xxx move some of this to init
    pixels = sdl_read_display_pixels(0, 0, sdl_win_width, sdl_char_height);
    write_file("unit_test_pixels", pixels, pixels->struct_len);
    free(pixels);

    pixels = read_file("unit_test_pixels", &file_length);
    if (pixels == NULL || pixels->struct_len != file_length) {
        printf("ERROR: failed to read unit_test_pixels\n");
        goto done;
    }
    t = sdl_create_texture_from_pixels(pixels);
    sdl_render_texture(0, 900, -1, -1, 0, t);
    sdl_destroy_texture(t);
    free(pixels);

done:
}

static void page_5_cleanup(void)
{
    sdl_destroy_texture(circle);
    sdl_destroy_texture(text);
}



// -----------------  PAGE 6: COLORS  -------------------------

static void color_test(int idx, char *color_name, int color);

static void render_page_6(bool init)
{
    int idx = 0;

    color_test(idx++, "WHITE", COLOR_WHITE);
    color_test(idx++, "RED",   COLOR_RED);
    color_test(idx++, "ORANGE", COLOR_ORANGE);
    color_test(idx++, "YELLOW", COLOR_YELLOW);
    color_test(idx++, "GREEN", COLOR_GREEN);
    color_test(idx++, "BLUE", COLOR_BLUE);
    color_test(idx++, "INDIGO", COLOR_INDIGO);
    color_test(idx++, "VIOLET", COLOR_VIOLET);
    color_test(idx++, "PURPLE", COLOR_PURPLE);
    color_test(idx++, "LIGHT_BLUE", COLOR_LIGHT_BLUE);
    color_test(idx++, "PINK", COLOR_PINK);
    color_test(idx++, "TEAL", COLOR_TEAL);
    color_test(idx++, "LIGHT_GRAY", COLOR_LIGHT_GRAY);
    color_test(idx++, "GRAY", COLOR_GRAY);
    color_test(idx++, "DARK_GRAY", COLOR_DARK_GRAY);
}

static void color_test(int idx, char *color_name, int color)
{
    int y = 2 * sdl_char_height + idx * 100;

    sdl_render_text(0, y, color_name);
    sdl_render_fill_rect(500, y, 500, sdl_char_height, color);
}

