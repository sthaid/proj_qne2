#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include <sdl.h>
#include <utils.h>

#include "tester.h"

//
// defines
//

#define MAX_PAGE 9

// xxx check these
#define ROW2Y(r) ((r) * sdl_char_height)  // xxx ctr vs ...
#define ROW2Y_CTR(r) ((r) * sdl_char_height + sdl_char_height/2)
#define NK2X(n,k) ((sdl_win_width/2/(n)) + (k) * (sdl_win_width/(n)))

// events common to all pages
#define EVID_PREV_PAGE   1
#define EVID_NEXT_PAGE   2
#define EVID_END_PROGRAM 3

//
// variables
//

static bool end_program;

//
// prototypes
//

static void page_hndlr(void);

static void page_0_draw(void);

static void page_1_draw(void);

static void page_2_draw(void);

static void page_3_init(void);
static void page_3_draw(void);
static void page_3_process_event(sdl_event_t *event);
static void page_3_exit(void);

static void page_4_draw(void);

static void page_5_init(void);
static void page_5_draw(void);
static void page_5_exit(void);

static void page_6_draw(void);

static void page_7_init(void);
static void page_7_draw(void);
static void page_7_process_event(sdl_event_t *event);

static void page_8_init(void);
static void page_8_draw(void);
static void page_8_exit(void);

// -----------------  MAIN  ------------------------------------------

int main(int argc, char **argv)
{
    bool is_ez_app = (argc > 0 && strcmp(argv[0], "ez_app") == 0);

    // print args 
    printf("argc = %d\n", argc);
    for (int i = 0; i < argc; i++) {
        printf("argv[%d] = '%s'\n", i, argv[i]);
    }
    printf("is_ez_app = %d\n", is_ez_app);

    // if not ez_app then call sdl_init
    if (!is_ez_app && sdl_init() != 0) {
        printf("ERROR: sdl_init failed\n");
        return 1;
    }

    // print window and char sized, these are global variables from sdl.c;
    // the initial char size provides 20 chars across the display width
    printf("sdl_win_width/height  = %d %d\n", sdl_win_width, sdl_win_height);
    printf("sdl_char_width/height = %d %d\n", sdl_char_width, sdl_char_height);

    // test calling a routine that is defined in another file
    tester_proc();

    // call handler routine for the current page
    while (true) {
        page_hndlr();
        if (end_program) {
            break;
        }
    }

    // if not ez_app then call sdl_exit
    if (!is_ez_app) {
        sdl_exit();
    }

    // return success
    return 0;
}

// -----------------  SUPPORT PROCS FOR ALL PAGES  ------------

// xxx check this
// picoc: picoc does not support this being static, causes crash
char *page_title[] = {       // Page
        "Unit Test",    //   0
        "Font",         //   1
        "Sizeof",       //   2
        "Multi Lines",  //   3
        "Drawing",      //   4
        "Textures",     //   5
        "Colors",       //   6
        "Audio",        //   7
        "Sensors",      //   8
            };
static int pagenum = 0;

static void page_hndlr()
{
    sdl_event_t event;
    sdl_loc_t  *loc;
    int         new_pagenum = -1;

    sdl_print_init(20, COLOR_WHITE, COLOR_BLACK);

    // call the page specific init routine, if provided
    switch (pagenum) {
    case 3: page_3_init(); break;
    case 5: page_5_init(); break;
    case 7: page_7_init(); break;
    case 8: page_8_init(); break;
    }

    while (true) {
        // init the backbuffer
        sdl_display_init(COLOR_BLACK);

        // draw title line
        sdl_print_init(20, COLOR_WHITE, COLOR_BLACK);
        sdl_render_text_xyctr(NK2X(1,0), ROW2Y_CTR(0), page_title[pagenum]);

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
        sdl_print_init(20, COLOR_WHITE, COLOR_BLACK);

        // draw display
        switch (pagenum) {
        case 0: page_0_draw(); break;
        case 1: page_1_draw(); break;
        case 2: page_2_draw(); break;
        case 3: page_3_draw(); break;
        case 4: page_4_draw(); break;
        case 5: page_5_draw(); break;
        case 6: page_6_draw(); break;
        case 7: page_7_draw(); break;
        case 8: page_8_draw(); break;
        default:
            printf("ERROR invalid pagenum %d\n", pagenum);
            end_program = true;
            return;
        }

        // present the display
        sdl_display_present();

        // wait for an event with 50 ms timeout;
        // if no event available, then continue
        sdl_get_event(50000, &event);
        if (event.event_id == -1) {
            continue;
        }

        // process common events
        switch (event.event_id) {
        case EVID_QUIT: case EVID_END_PROGRAM:
            end_program = true;
            break;      
        case EVID_SWIPE_RIGHT: case EVID_PREV_PAGE:
            new_pagenum = pagenum - 1;
            if (new_pagenum < 0) {
                new_pagenum = MAX_PAGE-1;
            }
            break;      
        case EVID_SWIPE_LEFT: case EVID_NEXT_PAGE:
            new_pagenum = pagenum + 1;
            if (new_pagenum >= MAX_PAGE) {
                new_pagenum = 0;
            }
            break;      
        }

        // if the page has been changed or the program is terminating
        // then break out of the loop
        if (new_pagenum != -1 || end_program) {
            break;
        }

        // it wasn't a common event;
        // call the page specific event hndlr, if provided
        switch (pagenum) {
        case 3: page_3_process_event(&event); break;
        case 7: page_7_process_event(&event); break;
        }
    }

    // call the page specific exit routine, if provided
    switch (pagenum) {
    case 3: page_3_exit(); break;
    case 5: page_5_exit(); break;
    case 8: page_8_exit(); break;
    }

    // update pagenum
    pagenum = new_pagenum;
}

// -----------------  PAGE 0: CLOCK  --------------------------

static void page_0_draw(void)
{
    time_t t;
    struct tm *tm;
    char str[100];
    long usecs, delta;
    static long usecs_last, usecs_first;
    
    // print the time, hh:mm:ss
    time(&t);
    tm = localtime(&t);
    sprintf(str, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
    sdl_render_text_xyctr(sdl_win_width/2, ROW2Y(5), str);

    // print the time in microsecs
    usecs = util_get_real_time_us();
    util_time2str(str, usecs, false, true, false);
    sdl_render_text_xyctr(sdl_win_width/2, ROW2Y(7), str);

    // print microsecs since this page is first viewed, and
    // print the delta time since last display update
    usecs = util_microsec_timer();
    if (usecs_first == 0) {
        usecs_first = usecs;
    }
    delta = usecs - usecs_last;
    usecs_last = usecs;
    sdl_render_printf_xyctr(sdl_win_width/2, ROW2Y(9), "%0.3f %ld", (usecs-usecs_first)/1000000., delta);
}

// -----------------  PAGE 1: FONT  ---------------------------

static void page_1_draw(void)
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

static void page_2_draw(void)
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

// xxx not working correctly, probably scaling problem

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

static void page_3_init(void)
{
    char *p = lines;
    for (int i = 0; i < 100; i++) {
        p += sprintf(p, "Line %d\n", i);
    }

    for (int i = 0; i < 100; i++) {
        lines_2[i] = malloc(20);
        sprintf(lines_2[i], "Line-V2 %d", i);
    }

    y_top = ROW2Y(2); 
    y_display_begin = ROW2Y(2);
    y_display_end = sdl_win_height-3*sdl_char_height;
    test_v1 = !test_v1;
}

static void page_3_draw(void)
{
    sdl_register_event(NULL, EVID_MOTION);

    if (test_v1) {
        sdl_render_multiline_text(y_top, y_display_begin, y_display_end, lines);
    } else {
        sdl_render_multiline_text_2(y_top, y_display_begin, y_display_end, lines_2, 100);
    }
}

static void page_3_process_event(sdl_event_t *event)
{
    if (event->event_id == EVID_MOTION) {
        y_top += event->u.motion.yrel;
        if (y_top >= y_display_begin) {
            y_top = y_display_begin;
        }
    }
}

static void page_3_exit(void)
{
    for (int i = 0; i < 100; i++) {
        free(lines_2[i]);
        lines_2[i] = NULL;
    }
}

// -----------------  PAGE 4: DRAWING  ------------------------

static void add_point(sdl_point_t **p, int x, int y);

static void page_4_draw(void)
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

static void page_5_init(void)
{
    circle = sdl_create_filled_circle_texture(100, COLOR_RED);
    text   = sdl_create_text_texture("XXXXX");
}

static void page_5_draw(void)
{
    int ret, w, h, file_length;
    sdl_texture_t *t;
    sdl_pixels_t *pixels;

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

    // create unit_test_pixels file from the top row of the display
    pixels = sdl_read_display_pixels(0, 0, sdl_win_width, sdl_char_height);
    if (pixels == NULL || pixels->magic != PIXELS_MAGIC) {
        printf("ERROR: failed to read unit_test_pixels, pixels==NULL\n");
    } else {
        ret = util_write_file("unit_test_pixels", pixels, pixels->struct_len);
        if (ret != 0) {
            printf("ERROR: failed to write file unit_test_pixels\n");
        }
    }
    free(pixels);
    pixels = NULL;

    // read the unit_test_pixels file that as created above
    // create a texture from the pixels, and
    // display the texture
    pixels = util_read_file("unit_test_pixels", &file_length);
    if (pixels == NULL || pixels->magic != PIXELS_MAGIC || pixels->struct_len != file_length) {
        if (pixels == NULL) {
            printf("ERROR: failed to read unit_test_pixels, pixels==NULL\n");
        } else {
            printf("ERROR: unit_test_pixels file invalid, magic=0x%x struct_len=%d file_length=%d\n",
                   pixels->magic, pixels->struct_len, file_length);
        }
    } else {
        t = sdl_create_texture_from_pixels(pixels);
        sdl_render_texture(0, 900, -1, -1, 0, t);
        sdl_destroy_texture(t);
    }
    free(pixels);
    pixels = NULL;

    // delete unit_test_pixels
    unlink("unit_test_pixels");
}

static void page_5_exit(void)
{
    sdl_destroy_texture(circle);
    sdl_destroy_texture(text);
}

// -----------------  PAGE 6: COLORS  -------------------------

static void color_test(int idx, char *color_name, int color);

static void page_6_draw(void)
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

// -----------------  PAGE 7: AUDIO  --------------------------

// xxx stop audio when exit page

#define EVID_AUDIO_PLAY_TONE        10
#define EVID_AUDIO_PLAY_FREQ_SWEEP  11
#define EVID_AUDIO_PLAY_SQUARE_WAVE 12
#define EVID_AUDIO_PLAY_MORSE_CODE  13
#define EVID_AUDIO_PLAY_RECORDING   19
#define EVID_AUDIO_RECORD           20
#define EVID_AUDIO_STOP             30
#define EVID_AUDIO_PAUSE            31
#define EVID_AUDIO_CONT             32

static void add_tone(sdl_tone_t **t, int freq, int intvl);
static void add_gap(sdl_tone_t **t, int intvl);
static void add_terminator(sdl_tone_t **t);
static char *audio_state_str(int x);
static void generate_morse_code_tones(sdl_tone_t **t, char *letters);
       
static void page_7_init(void)
{
    sdl_audio_print_devices_info();
}

static void page_7_draw(void)
{
    sdl_loc_t *loc;
    sdl_audio_state_t state;
    int y;

    //
    // get audio state
    //

    sdl_audio_state(&state);

    //
    // record section
    //

    sdl_print_init(-1, state.state == AUDIO_STATE_RECORD ? COLOR_RED : COLOR_WHITE, COLOR_BLACK);
    loc = sdl_render_text(0, 200, "RECORD");
    sdl_register_event(loc, EVID_AUDIO_RECORD);
    sdl_print_init(-1, COLOR_WHITE, COLOR_BLACK);

    //
    // play section
    //

    y = 400;

    sdl_render_text_xyctr(NK2X(1,0), y, "--- PLAY ---");
    y += 150;

    loc = sdl_render_text(0, y, "RECORDING");
    sdl_register_event(loc, EVID_AUDIO_PLAY_RECORDING);
    y += 150;

    loc = sdl_render_text(0, y, "TONE");
    sdl_register_event(loc, EVID_AUDIO_PLAY_TONE);
    y += 150;

    loc = sdl_render_text(0, y, "FREQ_SWEEP");
    sdl_register_event(loc, EVID_AUDIO_PLAY_FREQ_SWEEP);
    y += 150;

    loc = sdl_render_text(0, y, "SQUARE_WAVE");
    sdl_register_event(loc, EVID_AUDIO_PLAY_SQUARE_WAVE);
    y += 150;

    loc = sdl_render_text(0, y, "MORSE_CODE");
    sdl_register_event(loc, EVID_AUDIO_PLAY_MORSE_CODE);
    y += 150;

    //
    // state section
    //

    if (state.state != AUDIO_STATE_IDLE) {
        y = sdl_win_height-650;

        // state, processed/total time, and paused
        sdl_render_printf(0, y, "%s %d / %d", 
                          audio_state_str(state.state), state.processed_secs, state.total_secs);
        if (state.paused) {
            sdl_render_printf(sdl_win_width-sdl_char_width, y, "%s", "P");
        }
        y += sdl_char_height;

        // volume
        sdl_render_fill_rect(0, y, sdl_win_width * state.volume / 100, sdl_char_height, COLOR_RED);
        sdl_render_rect(0, y, sdl_win_width, sdl_char_height, 2, COLOR_WHITE);
        y += sdl_char_height;

        // filename
        if (state.filename[0]) {
            sdl_render_printf(0, y, "%s", state.filename);
            y += sdl_char_height;
        }

    }

    //
    // stop, pause, cont controls section
    //

    loc = sdl_render_text(0, sdl_win_height-300, "STOP");
    sdl_register_event(loc, EVID_AUDIO_STOP);

    loc = sdl_render_text(sdl_win_width/2-2.5*sdl_char_width, sdl_win_height-300, "PAUSE");
    sdl_register_event(loc, EVID_AUDIO_PAUSE);

    loc = sdl_render_text(sdl_win_width-4*sdl_char_width, sdl_win_height-300, "CONT");
    sdl_register_event(loc, EVID_AUDIO_CONT);
}

static void page_7_process_event(sdl_event_t *ev)
{
    int rc, i, freq;
    sdl_tone_t tones[5000];
    sdl_tone_t *t;

    switch (ev->event_id) {
    case EVID_AUDIO_PLAY_TONE:
        sdl_audio_create_test_file("audio_test.raw", 10, 1000);
        rc = sdl_audio_play("audio_test.raw");
        if (rc != 0) {
            printf("ERROR: sdl_audio_play audio_test.raw failed\n");
        }
        unlink("audio_test.raw");
        break;
    case EVID_AUDIO_PLAY_RECORDING:
        rc = sdl_audio_play("recording.raw");
        if (rc != 0) {
            printf("ERROR: sdl_audio_play recording.raw failed\n");
        }
        break;
    case EVID_AUDIO_PLAY_FREQ_SWEEP:
        t = tones;
        for (freq = 100; freq <= 3000; freq += 100) {
            add_tone(&t, freq, 1);
        }
        add_terminator(&t);
        sdl_audio_play_tones(500, tones); // intvl=500 ms
        break;
    case EVID_AUDIO_PLAY_SQUARE_WAVE:
        t = tones;
        for (i = 0; i < 10; i++) {
            add_tone(&t, 500, 1);
            add_gap(&t, 1);
        }
        add_terminator(&t);
        sdl_audio_play_tones(500, tones);
        break;
    case EVID_AUDIO_PLAY_MORSE_CODE:
        t = tones;
        generate_morse_code_tones(&t, "CQ CQ HELLO WORLD CQ CQ");
        sdl_audio_play_tones(100, tones); // intvl = 100 ms
        break;
    case EVID_AUDIO_RECORD: {
        char *record_file_name = "recording.raw";
        sdl_audio_state_t state;

        sdl_audio_state(&state);
        if (state.state != AUDIO_STATE_RECORD) {  //xxx add ING to name end  - 'RECORDING'
            rc = sdl_audio_record(record_file_name, 30, false);
            if (rc != 0) {
                printf("ERROR: sdl_audio_record %s failed\n", record_file_name);
            }
        } else {
            sdl_audio_ctl(AUDIO_REQ_STOP);
        }
        break; }
    case EVID_AUDIO_STOP:
        sdl_audio_ctl(AUDIO_REQ_STOP);
        break;
    case EVID_AUDIO_PAUSE:
        sdl_audio_ctl(AUDIO_REQ_PAUSE);
        break;
    case EVID_AUDIO_CONT:
        sdl_audio_ctl(AUDIO_REQ_UNPAUSE);
        break;
    }
}

static char *audio_state_str(int x)
{
    if (x == AUDIO_STATE_IDLE)       return "IDLE";
    if (x == AUDIO_STATE_PLAY_FILE)  return "PLAY_FILE";
    if (x == AUDIO_STATE_PLAY_TONES) return "PLAY_TONES";
    if (x == AUDIO_STATE_RECORD)     return "RECORD";
    return "INVLD_STATE";
}

static void add_tone(sdl_tone_t **t, int freq, int intvl)
{
    (*t)->freq = freq;
    (*t)->intvl = intvl;
    *t = *t + 1;
}

static void add_gap(sdl_tone_t **t, int intvl)
{
    (*t)->freq = 0;
    (*t)->intvl = intvl;
    *t = *t + 1;
}

static void add_terminator(sdl_tone_t **t)
{
    (*t)->freq = 0;
    (*t)->intvl = 0;
    *t = *t + 1;
}

#define MORSE_FREQ 1000

static void generate_morse_code_tones(sdl_tone_t **t, char *letters)
{
    char *morse_chars[] = {
                    /* A */ ".-",      /* B */ "-...",    /* C */ "-.-.",
                    /* D */ "-..",     /* E */ ".",       /* F */ "..-.",
                    /* G */ "--.",     /* H */ "....",    /* I */ "..",
                    /* J */ ".---",    /* K */ "-.-",     /* L */ ".-..",
                    /* M */ "--",      /* N */ "-.",      /* O */ "---",
                    /* P */ ".--.",    /* Q */ "--.-",    /* R */ ".-.",
                    /* S */ "...",     /* T */ "-",       /* U */ "..-",
                    /* V */ "...-",    /* W */ ".--",     /* X */ "-..-",
                    /* Y */ "-.--",    /* Z */ "--..", };

    for (int i = 0; letters[i]; i++) {
        int ch = letters[i];
        if (ch >= 'A' && ch <='Z') {
            for (int j = 0; morse_chars[ch-'A'][j]; j++) {
                int intvl = (morse_chars[ch-'A'][j] == '.') ? 1 : 3;
                add_tone(t, MORSE_FREQ, intvl);
                add_gap(t, 1);
            }
            add_gap(t, 2);  // xxx update_gap
        } else if (ch == ' ') {
            add_gap(t, 4);
        }
    }
    add_terminator(t);
}

// -----------------  PAGE 8: SENSORS -------------------------

int sensor_id = -1;

static void page_8_init(void)
{
    sensor_id = sdl_sensor_open(true, ASENSOR_TYPE_STEP_COUNTER);
}

static void page_8_draw(void)
{
    double value = 0;

    if (sensor_id < 0) {
        sdl_render_printf(0, 200, "open failed");
        return;
    }

    sdl_sensor_read(sensor_id, &value);
    sdl_render_printf(0, 200, "id=%d val=%f", sensor_id, value);
}

static void page_8_exit(void)
{
    if (sensor_id >= 0) {
        sdl_sensor_close(sensor_id);
    }
}

