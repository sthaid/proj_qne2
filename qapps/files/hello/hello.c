#if 0
picoc.c arg0
hello.c
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

int w,h;

static void render_page(int n);
static void print_setup(int ptsize_arg, int fg_color_arg, int bg_color_arg);
static sdl_rect_t *print_text(int x, int y, char *str);
static sdl_rect_t *print_text_nk(int n, int k, int y, char *str);

int main(int argc, char **argv)
{
    int i, rc;
    //int fcw, fch;
    int pagenum=0, event_id;
    bool end_program = false;

    // font char height = ptsize
    //      char width = 0.6 * ptsize
    printf("argc = %d\n", argc);
    for (i = 0; i < argc; i++) {
        printf("argv[%d] = '%s'\n", i, argv[i]);
    }

    rc = sdl_init(&w, &h);
    printf("sdl_init rc=%d\n", rc);

    print_setup(40, COLOR_WHITE, COLOR_BLACK);
#if 0
    for (i = 10; i < MAX_FONT_PTSIZE; i++) {
        sdl_get_char_size(i, &fcw, &fch);
        printf("font %3d - %3d x %3d\n", i, fcw, fch);
    }
#endif

    while (!end_program) {
        // xxx reset other stuff here too, fontsz, color
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

    sdl_exit();
    return 0;
}

// -----------------  PRINT SUPPORT  --------------------------

// xxx move to sdl ?
static int ptsize;
static int fg_color;
static int bg_color;
static int char_width;
static int char_height;
static int rows;
static int cols;

#define ROW2Y(r) ((r) * char_height)

static int XYZ(int n, int k, char *str)
{
    return ((w/2/(n)) + (k) * (w/(n)) - strlen(str) *char_width / 2);
}

static void print_setup(int ptsize_arg, int fg_color_arg, int bg_color_arg)
{
    ptsize = ptsize_arg;
    fg_color = fg_color_arg;
    bg_color = bg_color_arg;

    sdl_get_char_size(ptsize, &char_width, &char_height);

    rows = h / char_height;
    cols = w / char_width;
}

static sdl_rect_t *print_text(int x, int y, char *str)
{
    return sdl_render_text(x, y, ptsize, fg_color, bg_color, str);
}

static sdl_rect_t *print_text_nk(int n, int k, int y, char *str)
{
    return sdl_render_text(XYZ(n,k,str), y, ptsize, fg_color, bg_color, str);
}

// -----------------  XXXXXXXXXXXXX  --------------------------

static void render_page_0(void);
static void render_page_1(void);

static void render_page(int pagenum)
{
    sdl_rect_t *loc;
    char str[100];

    sprintf(str, "Hello Page %d", pagenum);
    print_text_nk(1, 0, ROW2Y(0), str);

    loc = print_text_nk(3, 0, ROW2Y(rows-1), "<");
    sdl_register_event(loc, EVID_PAGE_DECREMENT);

    loc = print_text_nk(3, 1, ROW2Y(rows-1), ">");
    sdl_register_event(loc, EVID_PAGE_INCREMENT);

    loc = print_text_nk(3, 2, ROW2Y(rows-1), "X");
    sdl_register_event(loc, EVID_END_PROGRAM);

    switch (pagenum) {
    case 0: render_page_0(); break;
    case 1: render_page_1(); break;
    }
}

static void render_page_0(void)
{
    time_t t;
    int r = rows/3;
    struct tm *tm;
    char str[100];

    time(&t);
    tm = localtime(&t);

    sprintf(str, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
    print_text_nk(1, 0, ROW2Y(r), str);
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

        print_text(0, ROW2Y(i+2), str);
        ch += 16;
    }
}

#if 0
  printf("sizoef(char)      = %zd\n", sizeof(char));
    printf("sizoef(short)     = %zd\n", sizeof(short));
    printf("sizoef(int)       = %zd\n", sizeof(int));
    printf("sizoef(long)      = %zd\n", sizeof(long));
    printf("sizoef(size_t)    = %zd\n", sizeof(size_t));
    printf("sizoef(off_t)     = %zd\n", sizeof(off_t));
    printf("sizoef(time_t)    = %zd\n", sizeof(time_t));
    printf("sizeof(123)       = %zd\n", sizeof(123));
    printf("sizeof(123UL)     = %zd\n", sizeof(123UL));
#endif
