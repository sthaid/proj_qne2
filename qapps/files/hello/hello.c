#if 0
hello.c outline
- unit test 
- swipe to change screens
- screen 0 = clock
- screen 1 = font
- screen 2 = render text and rectangles, etc
- screen 3 = textures
- screen 4 = sizeof prints
#endif

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#define MAX_PAGE 5

#define MAX_FONT_PTSIZE 200 // xxx global?

#define EVID_END_PROGRAM       1
#define EVID_SWIPE_UP          2
#define EVID_PAGE_DECREMENT    3
#define EVID_SWIPE_RIGHT       4
#define EVID_PAGE_INCREMENT    5
#define EVID_SWIPE_LEFT        6

#include <sdl.h>

static int win_width;
static int win_height;
static int char_width;
static int char_height;
static int win_rows;
static int win_cols;

static void page_4_init(void);
static void page_4_exit(void);


#define ROW2Y(r) ((r) * char_height)

static void render_page(int n);

int main(int argc, char **argv)
{
    int  i, rc, event_id;
    int  pagenum = 4; //XXX xxx
    bool end_program = false;
    bool is_qne_app = (argc > 0 && strcmp(argv[0], "qne_app") == 0);

    // xxx temp debug prints
    printf("argc = %d\n", argc);
    for (i = 0; i < argc; i++) {
        printf("argv[%d] = '%s'\n", i, argv[i]);
    }
    printf("is_qne_app = %d\n", is_qne_app);

    // if not qne_app then call sdl_init
    if (!is_qne_app && sdl_init() != 0) {
        return 1;
    }
    sdl_get_win_size(&win_width, &win_height);
    printf("win_width/height = %d %d\n", win_width, win_height);

    // xxx
    sdl_print_init(20, COLOR_WHITE, COLOR_BLACK, 
                   &char_width, &char_height, &win_rows, &win_cols);
    printf("font char width x height = %3d x %3d, win rows x cols = %d %d\n", 
               char_width, char_height, win_rows, win_cols);

    page_4_init();

    while (!end_program) {
        // xxx reset other stuff here too, fontsz, color
        sdl_display_init(COLOR_BLACK);

        // xxx
        render_page(pagenum);

        // update the display
        sdl_display_present();

        // wait for an event with 100 ms timeout;
        // if no event then redraw display
        event_id = sdl_get_event(10000); //xxx 10ms
        if (event_id == -1) {
            continue;
        }

        // process event
        printf("get event %d\n", event_id);
        switch (event_id) {
        case EVID_END_PROGRAM:
        case EVID_SWIPE_UP:
            end_program = true;
            break;
        case EVID_PAGE_DECREMENT:
        case EVID_SWIPE_RIGHT:
            if (pagenum > 0) pagenum--;
            break;
        case EVID_PAGE_INCREMENT:
        case EVID_SWIPE_LEFT:
            if (pagenum < MAX_PAGE-1) pagenum++;
            break;
        }
    }

    // xxx destory all
    page_4_exit();

    // if not qne_app then call sdl_exit
    if (!is_qne_app) {
        sdl_exit();
    }

    // return success
    return 0;
}

// -----------------  XXXXXXXXXXXXX  --------------------------

static void render_page_0(void);
static void render_page_1(void);
static void render_page_2(void);
static void render_page_3(void);
static void render_page_4(void);

static void render_page(int pagenum)
{
    sdl_rect_t *loc;
    char str[100];

    sprintf(str, "Hello Page %d", pagenum);
    sdl_render_printf_nk(1, 0, ROW2Y(0), "%s", str);

    loc = sdl_render_printf_nk(3, 0, ROW2Y(win_rows-1), "%s", "<");
    sdl_register_event(loc, EVID_PAGE_DECREMENT);

    loc = sdl_render_printf_nk(3, 1, ROW2Y(win_rows-1), "%s", ">");
    sdl_register_event(loc, EVID_PAGE_INCREMENT);

    loc = sdl_render_printf_nk(3, 2, ROW2Y(win_rows-1), "%s", "X");
    sdl_register_event(loc, EVID_END_PROGRAM);

    //sdl_render_text(0, ROW2Y(1), "123456789x123456789x123456789x123456789x");

    switch (pagenum) {
    case 0: render_page_0(); break;
    case 1: render_page_1(); break;
    case 2: render_page_2(); break;
    case 3: render_page_3(); break;
    case 4: render_page_4(); break;
    }
}

static void render_page_0(void)
{
    time_t t;
    int r = win_rows/3;
    struct tm *tm;
    char str[100];

    time(&t);
    tm = localtime(&t);

    sprintf(str, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
    sdl_render_printf_nk(1, 0, ROW2Y(r), "%s", str);

    // xxx intro text
    // - following pages sameple code
    // - swipe right, left, up
}

static void render_page_1(void)
{
    int i, ch=0;
    char str[32];

    for (i = 0; i < 16; i++) {
        sprintf(str, "%02x %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
                i*16,
                ch+0, ch+1, ch+2, ch+3, ch+4, ch+5, ch+6, ch+7,
                ch+8, ch+9, ch+10, ch+11, ch+12, ch+13, ch+14, ch+15);

        sdl_render_text(0, ROW2Y(i+2), str);  // xxx make nk version too
        ch += 16;
    }
}

static void render_page_2(void)
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

    static int pts[] = {
        10, 500 ,
        990, 500 ,
        990, 1500 ,
            };


void *xyz(void *x)
{
    return x;
}

    #define DELTA_INTEN 0.01
    static double inten;

static double wavelen = 440;
#define DELTA_WAVELEN 5

static void render_page_3(void)
{
    sdl_rect_t loc;
    
    loc.x = loc.y = 0;
    loc.w = 1000; loc.h = 1800;
    loc.w = 1000; loc.h = 2166;
    sdl_render_rect(&loc, 4, COLOR_PURPLE);

    loc.x = 100; loc.y = 100; loc.w = 800; loc.h = 100;
    sdl_render_fill_rect(&loc, COLOR_RED);


    sdl_render_circle(500, 350, 100, 10, COLOR_ORANGE);

    // xxx
    //sdl_render_line(0, 500, 1000, 500, COLOR_WHITE);

    //printf("count %d   %d %d\n", (int)(sizeof(pts)/(sizeof(sdl_point_t))),
        //(int)sizeof(pts), (int)sizeof(sdl_point_t));
    sdl_point_t pts[8];
    pts[0].x = 100; pts[0].y = 500;
    pts[1].x = 900; pts[1].y = 500;
    pts[2].x = 900; pts[2].y = 1500;
    pts[3].x = 100; pts[3].y = 1500;
    pts[4].x = 100; pts[4].y = 500;
    pts[5].x = 900; pts[5].y = 1500;
    pts[6].x = 100; pts[6].y = 1500;
    pts[7].x = 900; pts[7].y = 500;
    sdl_render_lines(pts, 8, COLOR_RED);

    sdl_render_line(50, 1550, 950, 1550, COLOR_WHITE);
    sdl_render_line(50, 1600, 950, 1600, COLOR_WHITE);

    //400, 1650, 200, 200
    int color;
#if 0
    static double inten[] = {0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
    static int idx;
    int color;
    color = sdl_scale_color(COLOR_YELLOW, inten[idx]);
    idx = (idx + 1) % 11;
    printf("idx = %d  color=0x%x\n", idx, color);
#endif

    inten += DELTA_INTEN;
    printf("INTEN %f\n", inten);
//  if (inten > 1) {
//      printf("set to zero\n");
//      inten = 0;
//  }
    if (inten > 1) inten = 0;
    color = sdl_scale_color(COLOR_YELLOW, inten);
    printf("COLOR 0x%x\n", color);

    loc.x = 100; loc.y = 1650; loc.w = 200; loc.h = 200;
    sdl_render_fill_rect(&loc, color);


    wavelen += DELTA_WAVELEN;
    if (wavelen > 750) wavelen = 440;
    color = sdl_wavelength_to_color(wavelen);

    loc.x = 700; loc.y = 1650; loc.w = 200; loc.h = 200;
    sdl_render_fill_rect(&loc, color);

// xxx test this
//int sdl_create_color(int r, int g, int b, int a);
}

// ----------------------------------

sdl_texture_t *circle;
sdl_texture_t *text;
int circle_x=400, circle_y=200;
int text_x=0, text_y=800;

double angle = 0;

sdl_texture_t *t1;

#define DELTA_CIRCLE_X 10

static void render_page_4(void)
{
#if 0
// render using textures
sdl_texture_t *sdl_create_texture(int w, int h);                                      // k
sdl_texture_t *sdl_create_texture_from_display(sdl_rect_t *loc);                      // xxx needed?
sdl_texture_t *sdl_create_filled_circle_texture(int radius, int color);               // k
sdl_texture_t *sdl_create_text_texture(char *str);                                    // k
void sdl_destroy_texture(sdl_texture_t *texture);                                     // k 
void sdl_query_texture(sdl_texture_t *texture, int *width, int *height);              // k
void sdl_update_texture(sdl_texture_t *texture, char *pixels, int pitch);             // k   add more testing
void sdl_render_texture(int x, int y, sdl_texture_t *texture);                        // k
void sdl_render_scaled_texture(sdl_rect_t *dest, sdl_texture_t *texture);             // k
#endif

    sdl_render_texture(circle_x, circle_y, circle);
    circle_x += DELTA_CIRCLE_X;
    if (circle_x > 1000) circle_x = 0;

    sdl_rect_t dest;

    dest.x = 500 - 400/2;
    dest.y = 500;
    dest.w = 400;
    dest.h = 200;
    sdl_render_scaled_texture(&dest, circle);

    sdl_render_texture(450, 800, t1);  // xxx purple squae

    sdl_render_texture(0,1000, text);

    int w,h;
    sdl_query_texture(text, &w, &h);
    sdl_render_rotated_texture(500-w/2, 1300, angle, text);
    angle = angle + 1;
    if (angle > 360) angle -= 360;
}

static void page_4_init(void)
{
    int w, h;
    int *pixels;
    int i;

    circle = sdl_create_filled_circle_texture(100, COLOR_RED);
    sdl_query_texture(circle, &w, &h);
    printf("circle texture: w=%d h=%d\n", w, h);

    text = sdl_create_text_texture("hello");
    sdl_query_texture(text, &w, &h);
    printf("text texture: w=%d h=%d\n", w, h);

    t1 = sdl_create_texture(100, 100);
    pixels = malloc(100 * 100 * 4);
    memset(pixels, 0, 100*100*4);
    for (i = 0; i < 10000; i++) {
        pixels[i] = COLOR_PURPLE;
    }
    sdl_update_texture(t1, (char*)pixels, 100*4);
    free(pixels);
}

static void page_4_exit(void)
{
    sdl_destroy_texture(circle);
    sdl_destroy_texture(text);
    sdl_destroy_texture(t1);
}
