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
int char_width;
int char_height;
int win_rows;
int win_cols;
#define ROW2Y(r) ((r) * char_height)

static void render_page(int n);

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

    sdl_print_init(40, COLOR_WHITE, COLOR_BLACK, 
                   &char_width, &char_height, &win_rows, &win_cols);

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

// -----------------  XXXXXXXXXXXXX  --------------------------

static void render_page_0(void);
static void render_page_1(void);

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

    switch (pagenum) {
    case 0: render_page_0(); break;
    case 1: render_page_1(); break;
    }
}

           struct tmx {
               int tm_sec;    /* Seconds (0-60) */
               int tm_min;    /* Minutes (0-59) */
               int tm_hour;   /* Hours (0-23) */
               int tm_mday;   /* Day of the month (1-31) */
               int tm_mon;    /* Month (0-11) */
               int tm_year;   /* Year - 1900 */
               int tm_wday;   /* Day of the week (0-6, Sunday = 0) */
               int tm_yday;   /* Day in the year (0-365, 1 Jan = 0) */
               int tm_isdst;  /* Daylight saving time */
           };


static void render_page_0(void)
{
    time_t t;
    int r = win_rows/3;
    struct tm *tm;
    char str[100];

    time(&t);
    //tm = (struct tmx*)localtime(&t);
    tm = localtime(&t);

    sprintf(str, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
    sdl_render_printf_nk(1, 0, ROW2Y(r), "%s", str);
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
