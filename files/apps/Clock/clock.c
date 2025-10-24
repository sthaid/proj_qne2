#include <stdio.h>  // xxx
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <sdlx.h>
#include <utils.h>

#include "apps/Clock/common.h"

// xxx 
// - maybe pixels shoudl be unsigned int
// - options to:
//   - 24 hr time ?

// defines
#define XCTR_CLOCK 500
#define YCTR_CLOCK 600
#define W_CLOCK    1000
#define H_CLOCK    1000

// variables
static char *day[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static char *month[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

// prototypes
static void draw_analog_clock_face(void);
static void draw_analog_clock_hands(struct tm *tm);
int sunrise_sunset(int year, int month, int day, time_t *trise, time_t *tset); //xxx static

// xxx add ack to settings

// -----------------  MAIN  ------------------------------------------

int main(int argc, char **argv)
{
    sdlx_event_t    event;
    int             rc, y;
    bool            quit = false;
    time_t          t;
    struct tm       tm;
    char            sunrise_calc[50], sunset_calc[50], midday_calc[50];

    // save arg values
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s, wXh=%d %d\n", 
           progname, data_dir, sdlx_win_width, sdlx_win_height);

    // xxx
    //char sunrise_web[50], sunset_web[50], midday_web[50];
    //sunrise_sunset_web(sunrise_web, sunset_web, midday_web);
    //printf("INFO %s: WEB   %s %s %s\n", progname, sunrise_web, midday_web, sunset_web);

    // xxx
    sunrise_sunset_calc(sunrise_calc, sunset_calc, midday_calc);
    printf("INFO %s: CALC  %s %s %s\n", progname, sunrise_calc, midday_calc, sunset_calc);

    // init sdl video subsystem
    rc = sdlx_init(SUBSYS_VIDEO);
    if (rc != 0) {
        printf("ERROR %s: sdlx_init failed\n", progname);
        return 1;
    }

    // runtime loop
    while (!quit) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // register control event to end program
        sdlx_register_control_events(NULL, NULL, "X", COLOR_BLACK, 0, 0, EVID_QUIT);

        // xxx get the time
        t = time(NULL);
        localtime_r(&t, &tm);

        // display the analog clock  xxx just one call
        draw_analog_clock_face();
        draw_analog_clock_hands(&tm);

        // xxx
        sdlx_print_init(DEFAULT_FONT, COLOR_WHITE, COLOR_BLACK);

        // display the date and time below the analog clock, example:
        // 01:30:00 PM EDT  or  13:30:00 EDT
        // Wed Oct 21 2025
        y = YCTR_CLOCK + H_CLOCK / 2 + 1.5 * sdlx_char_height;
        sdlx_render_printf_xyctr(
                sdlx_win_width/2, y, 
                "%02d:%02d:%02d %s",
                tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_zone);
        y += 1.5 * sdlx_char_height;
        sdlx_render_printf_xyctr(
                sdlx_win_width/2, y, 
                "%s %s %d %d",
                day[tm.tm_wday], month[tm.tm_mon], tm.tm_mday, tm.tm_year+1900);
        y += 1.5 * sdlx_char_height;

        // display sunrise, midday, sunset times, example:
        // RISE     MID      SET
        // 07:00   12:00   17:00
        sdlx_render_printf(sdlx_char_width/2, y, "RISE");
        sdlx_render_printf(sdlx_win_width/2-3*sdlx_char_width/2, y, "MID");
        sdlx_render_printf(sdlx_win_width-4*sdlx_char_width, y, "SET");
        y += 1.5 * sdlx_char_height;
        sdlx_render_printf(0, y, "%s", sunrise_calc);
        sdlx_render_printf(sdlx_win_width/2-5*sdlx_char_width/2, y, "%s", midday_calc);
        sdlx_render_printf(sdlx_win_width-5*sdlx_char_width, y, "%s", sunset_calc);
        // xxx
    
        // present the display
        sdlx_display_present();

        // wait for an event with 1 s timeout;
        // if no event, then redraw display
        sdlx_get_event(1000000, &event);
        if (event.event_id == -1) {
            continue;
        }

        // process events
        switch (event.event_id) {
        case EVID_QUIT:
            quit = true;
            break;
        }
    }

    // cleanup and end program
    //xxx cleanup_analog_clock();
    sdlx_quit(SUBSYS_VIDEO);
    printf("INFO %s: terminating\n", progname);
    return 0;
}

// -----------------  ANALOG CLOCK FACE  -----------------------------

// xxx save the face in a texture

static void draw_analog_clock_face(void)
{
    int hour, x, y;

    sdlx_render_fill_rect(0, 100, 1000, 1000, COLOR_WHITE);

    sdlx_print_init(DEFAULT_FONT, COLOR_BLACK, COLOR_WHITE);

    for (hour = 1; hour <= 12; hour++) {
        x = XCTR_CLOCK + 400 * sin(hour * 30 * (M_PI / 180));  //xxx use deines
        y = YCTR_CLOCK - 400 * cos(hour * 30 * (M_PI / 180));
        sdlx_render_printf_xyctr(x, y, "%d", hour);
    }
}

// -----------------  ANALOG CLOCK HANDS -----------------------------

#define W_HH  34
#define H_HH  280
#define O_HH  40

#define W_MH  17
#define H_MH  375
#define O_MH  60

#define W_SH  4
#define H_SH  425
#define O_SH  100

sdlx_texture_t *hour_hand;
sdlx_texture_t *minute_hand;
sdlx_texture_t *second_hand;

static sdlx_texture_t * create_rectangle_texture(int w, int h, int color);

static void draw_analog_clock_hands(struct tm *tm)
{
    static bool first_call = true;

    double hour_hand_angle, minute_hand_angle, second_hand_angle;
    long   secs;

    if (first_call) {
        hour_hand = create_rectangle_texture(W_HH, H_HH, COLOR_BLACK);
        minute_hand = create_rectangle_texture(W_MH, H_MH, COLOR_BLACK);  // xxx free these
        second_hand = create_rectangle_texture(W_SH, H_SH, COLOR_RED);  // xxx free these
        first_call = false;
    }

    secs = 3600 * tm->tm_hour + 60 * tm->tm_min + tm->tm_sec;

    hour_hand_angle   = secs * (360. / (12 * 3600));
    minute_hand_angle = secs * (360. / 3600);
    second_hand_angle = secs * (360. / 60);

    sdlx_render_texture_ex2(XCTR_CLOCK-(W_HH/2), YCTR_CLOCK-H_HH+O_HH, 
                            W_HH, H_HH, 
                            hour_hand_angle, 
                            W_HH/2, H_HH-O_HH,
                            hour_hand);

    sdlx_render_texture_ex2(XCTR_CLOCK-(W_MH/2), YCTR_CLOCK-H_MH+O_MH, 
                            W_MH, H_MH, 
                            minute_hand_angle, 
                            W_MH/2, H_MH-O_MH,
                            minute_hand);

    sdlx_render_texture_ex2(XCTR_CLOCK-(W_SH/2), YCTR_CLOCK-H_SH+O_SH, 
                            W_SH, H_SH, 
                            second_hand_angle,
                            W_SH/2, H_SH-O_SH,
                            second_hand);

    sdlx_render_point(XCTR_CLOCK, YCTR_CLOCK, COLOR_RED, 9);
}
    
sdlx_texture_t * create_rectangle_texture(int w, int h, int color)
{
    unsigned int *pixels = malloc(w * h * BYTES_PER_PIXEL);
    sdlx_texture_t *t;

    pixels = malloc(w * h * BYTES_PER_PIXEL);
    for (int i = 0; i < w * h; i++) {
        pixels[i] = color;
    }
    t = sdlx_create_texture_from_pixels((unsigned char*)pixels, w, h);
    free(pixels);

    return t;
}

