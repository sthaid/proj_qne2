// This uapp provides unit test of the following:
// xxx
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include <sdl.h>

//
// defines
//

#define MAX_PAGE 5

// xxx check these
#define ROW2Y(r) ((r) * char_height)  // xxx ctr vs ...
#define ROW2Y_CTR(r) ((r) * char_height + char_height/2)

#define NK2X(n,k) ((win_width/2/(n)) + (k) * (win_width/(n)))

//
// variables
//

static int win_width;
static int win_height;
static int char_width;
static int char_height;
static int win_rows;
static int win_cols;

sdl_texture_t *circle;  // xxx can these be static
sdl_texture_t *text;
sdl_texture_t *purple;

//
// prototypes
//

static void page_4_cleanup(void);
static void render_page(int n);
static void render_page_0(void);
static void render_page_1(void);
static void render_page_2(void);
static void render_page_3(void);
static void render_page_4(void);

// -----------------  MAIN  ------------------------------------------

int main(int argc, char **argv)
{
    int  i, event_id;
    int  pagenum = 4;
    bool end_program = false;
    bool is_qne_app = (argc > 0 && strcmp(argv[0], "qne_app") == 0);

    // print args 
    printf("argc = %d\n", argc);
    for (i = 0; i < argc; i++) {
        printf("argv[%d] = '%s'\n", i, argv[i]);
    }
    printf("is_qne_app = %d\n", is_qne_app);

    // if not qne_app then call sdl_init
    if (!is_qne_app && sdl_init() != 0) {
        printf("ERROR: sdl_init failed\n");
        return 1;
    }

    // get windows size
    sdl_get_win_size(&win_width, &win_height);
    printf("win_width/height = %d %d\n", win_width, win_height);

#if 0
    // initialize font for 20 chars across display, white on black;
    // and print returned values
    sdl_print_init(20, COLOR_WHITE, COLOR_BLACK,
                   &char_width, &char_height, &win_rows, &win_cols);
    printf("font char width x height = %3d x %3d, win rows x cols = %d %d\n", 
               char_width, char_height, win_rows, win_cols);
#endif

    // loop, displaying the currently selected pagenum and proces events
    while (!end_program) {
        // xxx reset other stuff here too, fontsz, color  ??
        // xxx comment
        sdl_display_init(COLOR_BLACK);

        // xxx
        render_page(pagenum);

        // update the display
        sdl_display_present();

        // wait for an event with 100 ms timeout;
        // if no event then redraw display
        event_id = sdl_get_event(100000);
        if (event_id == -1) {
            continue;
        }

        // process event
        switch (event_id) {
        case EVID_QUIT:
        case EVID_SWIPE_UP:
            end_program = true;
            break;
        case EVID_SWIPE_RIGHT:
            if (--pagenum < 0) {
                pagenum = MAX_PAGE-1;
            }
            break;
        case EVID_SWIPE_LEFT:
            if (++pagenum >= MAX_PAGE) {
                pagenum = 0;
            }
            break;
        }

        // if end_program flag is set then break
        if (end_program) {
            break;
        }
    }

    // xxx destory all
    page_4_cleanup();

    // if not qne_app then call sdl_exit
    if (!is_qne_app) {
        sdl_exit();
    }

    // return success
    return 0;
}

// -----------------  RENDER PAGS PROC  -----------------------

static void render_page(int pagenum)
{
    sdl_loc_t *loc;
    char str[100];

    sprintf(str, "Unit Test - Page %d", pagenum);
    sdl_print_init(20, COLOR_WHITE, COLOR_BLACK,
                   &char_width, &char_height, &win_rows, &win_cols);
    sdl_render_printf(true, NK2X(1,0), ROW2Y_CTR(0), "%s", str);


    sdl_print_init(10, COLOR_WHITE, COLOR_BLACK,
                   &char_width, &char_height, &win_rows, &win_cols);

    loc = sdl_render_printf(true, NK2X(3,0), win_height-120, "%s", "<");
    loc->w = loc->h = 200;
    sdl_register_event(loc, EVID_SWIPE_RIGHT);

    loc = sdl_render_printf(true, NK2X(3,1), win_height-120, "%s", ">");
    loc->w = loc->h = 200;
    sdl_register_event(loc, EVID_SWIPE_LEFT);

    loc = sdl_render_printf(true, NK2X(3,2), win_height-120, "%s", "X");
    loc->w = loc->h = 200;
    sdl_register_event(loc, EVID_QUIT);


    sdl_print_init(20, COLOR_WHITE, COLOR_BLACK,
                   &char_width, &char_height, &win_rows, &win_cols);
    sdl_render_printf(true, win_width/2, win_height-char_height/2, "page %d", pagenum);
//  printf("win_width=%d win_height=%d char_width=%d char_height=%d\n",
//        win_width, win_height, char_width, char_height);

    switch (pagenum) {
    case 0: render_page_0(); break;
    case 1: render_page_1(); break;
    case 2: render_page_2(); break;
    case 3: render_page_3(); break;
    case 4: render_page_4(); break;
    }
}

// -----------------  PAGE 0: xxxxxxxxxxxxxxxxxxx  ------------

static void render_page_0(void)
{
    time_t t;
    struct tm *tm;
    char str[100];

    time(&t);
    tm = localtime(&t);

    sprintf(str, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
    sdl_render_printf(true, win_width/2,  ROW2Y_CTR(win_rows/3), "%s", str);

    // xxx intro text
}

// -----------------  PAGE 0: xxxxxxxxxxxxxxxxxxx  ------------

static void render_page_1(void)
{
    int i, ch=0;
    char str[32];

    for (i = 0; i < 16; i++) {
        sprintf(str, "%02x %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
                i*16,
                ch+0, ch+1, ch+2, ch+3, ch+4, ch+5, ch+6, ch+7,
                ch+8, ch+9, ch+10, ch+11, ch+12, ch+13, ch+14, ch+15);

        sdl_render_text(false, 0, ROW2Y(i+2), str);
        ch += 16;
    }
}

// -----------------  PAGE 0: xxxxxxxxxxxxxxxxxxx  ------------

static void render_page_2(void)
{
    int r = 2;

    sdl_render_printf(false, 0, ROW2Y(r++), "sizoef(char)   = %zd", sizeof(char));
    sdl_render_printf(false, 0, ROW2Y(r++), "sizoef(short)  = %zd", sizeof(short));
    sdl_render_printf(false, 0, ROW2Y(r++), "sizoef(int)    = %zd", sizeof(int));
    sdl_render_printf(false, 0, ROW2Y(r++), "sizoef(long)   = %zd", sizeof(long));
    sdl_render_printf(false, 0, ROW2Y(r++), "sizoef(size_t) = %zd", sizeof(size_t));
    sdl_render_printf(false, 0, ROW2Y(r++), "sizoef(off_t)  = %zd", sizeof(off_t));
    sdl_render_printf(false, 0, ROW2Y(r++), "sizoef(time_t) = %zd", sizeof(time_t));
    sdl_render_printf(false, 0, ROW2Y(r++), "sizeof(1)      = %zd", sizeof(123));
    sdl_render_printf(false, 0, ROW2Y(r++), "sizeof(1ULL);  = %zd", sizeof(123UL));
}

// -----------------  PAGE 0: xxxxxxxxxxxxxxxxxxx  ------------

static void add_point(sdl_point_t **p, int x, int y);

static void render_page_3(void)
{
    // draw rect around perimeter
    //sdl_render_rect(win_width/2, win_height/2, win_width, win_height, 2, COLOR_PURPLE);
    sdl_render_rect(0, 0, win_width, win_height, 2, COLOR_PURPLE);

    // draw fill rect, y = 100 .. 300
    sdl_render_fill_rect(100, 100, 800, 200, COLOR_RED);

    // draw circles, y = 300 .. 400
    sdl_render_circle(1*win_width/4, 350, 50, 3, COLOR_YELLOW);
    sdl_render_circle(2*win_width/4, 350, 50, 3, COLOR_YELLOW);
    sdl_render_circle(3*win_width/4, 350, 50, 3, COLOR_YELLOW);

    // draw 6 lines, y = 400 .. 500
    for (int y = 401; y <= 501; y += 20) {  //xxx
        sdl_render_line(0, y, 1000, y, COLOR_WHITE);
    }

    // draw 3 lines to make a triangle, y = 500 .. 700
    sdl_point_t pts[4], *ptsx=pts;
    add_point(&ptsx, 501, 501);
    add_point(&ptsx, 701, 701);
    add_point(&ptsx, 301, 701);
    add_point(&ptsx, 501, 501);
    sdl_render_lines(pts, 4, COLOR_RED);

    // draw 2 squares and vary intensity and wavelen, y = 700 .. 800
    static double inten;
    int color;
    inten = inten + 0.01;
    if (inten > 1) inten = 0;
    color = sdl_scale_color(COLOR_YELLOW, inten);
    sdl_render_fill_rect(200, 750, 100, 100, color);

    static double wavelen = 750;
    wavelen -= 2;
    if (wavelen < 440) wavelen = 750;
    color = sdl_wavelength_to_color(wavelen);
    sdl_render_fill_rect(800, 750, 100, 100, color);

    // draw points with varying size, y = 850
    color = sdl_create_color(0, 255, 0, 255);
    for (int pointsize = 0; pointsize <= 9; pointsize++) {
        sdl_render_point(pointsize*100+50, 850, color, pointsize);
    }

    // draw 10 points of the same size, y = 950
    sdl_point_t points[10];
    for (int i = 0; i < 10; i++) {
        points[i].x = i*100+50;
        points[i].y = 950;
    }
    sdl_render_points(points, 10, COLOR_PURPLE, 5);
}

static void add_point(sdl_point_t **p, int x, int y)
{
    (*p)->x = x;
    (*p)->y = y;
    (*p)++;
}

// -----------------  PAGE 0: xxxxxxxxxxxxxxxxxxx  ------------

// xxx test or replace this
//sdl_texture_t *sdl_create_texture_from_display(sdl_rect_t *loc);                      // xxx needed?

static void render_page_4(void)
{
    //int w,h;

    // if the circle texture has not been initialized then
    // init the 3 textures used by this test:
    // - circle: sdl_create_filled_circle_texture
    // - text:   sdl_create_text_texture
    // - purple: sdl_create_texture + sdl_update_texture xxx
    if (circle == NULL) {
        circle = sdl_create_filled_circle_texture(100, COLOR_RED);
        //sdl_query_texture(circle, &w, &h);
        //printf("circle texture w x h = %d %d,  expected 201 x 201\n", w, h);

        text = sdl_create_text_texture("hello");
        //sdl_query_texture(text, &w, &h);

#if 0
        int *pixels = malloc(100 * 100 * 4);
        for (int i = 0; i < 10000; i++) {
            pixels[i] = COLOR_PURPLE;
        }
        //purple = sdl_create_texture_from_pixels(100, 100, pixels);
        purple = sdl_create_texture_from_pixels(100, 100, NULL);
        free(pixels);
#endif

        purple = sdl_create_texture_from_pixels(100, 100, NULL);

        int *pixels = malloc(100 * 100 * 4);
        for (int i = 0; i < 10000; i++) {
            pixels[i] = COLOR_PURPLE;
        }

        sdl_update_texture(purple, pixels);
        free(pixels);
    }

    // render the circle texture at varying x location, y = 100 .. 300
    static int circle_x=100, circle_y=200;
    sdl_render_texture(circle_x, circle_y, 200, 200, 0, circle);
    circle_x += 10;
    if (circle_x > 900) circle_x = 100;

    // render the circle texture using scaling, y = 300 .. 500
    //sdl_render_scaled_texture(500, 400, 400, 200, circle);
    sdl_render_texture(100, 500, 200, 400, 0, circle);

    // render text texture, at y = 550
    sdl_render_texture(0, 1000, -1, -1, 0, text);

    // rotate and render the text texture at y = 550 .. 950
    static double angle = 0;
    angle += 5;
    sdl_render_texture(500, 500, -1, -1, angle, text);

    // render the purple texture, which was constructed from pixels, y = 950 .. 1050
    sdl_render_texture(500, 1500, -1, -1, 45, purple);



    // xxx
    static sdl_texture_t *t;
    if (t == NULL) {
        int w,h;
        printf("XXXXXXXXXX win_width, char_height = %d %d\n", win_width, char_height);
        t = sdl_create_texture_from_display(0,0,win_width, char_height);
        sdl_query_texture(t, &w, &h);
        printf("XXXXXXXXXXXXXX w h = %d %d\n", w, h);
    }
    sdl_render_texture(0, 1500, -1, -1, 0, t);
#if 0
    if (xyz != NULL) {
        sdl_query_texture(xyz, &w, &h);
        printf("rendering textur xyz %d %d\n", w, h);
        sdl_render_texture(500, 1500, xyz);
    }
#endif
}

static void page_4_cleanup(void)
{
    sdl_destroy_texture(circle);
    sdl_destroy_texture(text);
    sdl_destroy_texture(purple);
    //sdl_destroy_texture(xyz);
}
