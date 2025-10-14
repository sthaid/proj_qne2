#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include <sdlx.h>
#include <utils.h>

// xxx maybe pixels shoudl be unsigned int

// variables
static char *progname;
static char *data_dir;

// prototypes
static void draw_analog_clock_numbers(void);
static void draw_analog_clock_hands(void);
sdlx_texture_t * create_rectangle_texture(int w, int h, int color);

// select different faces
//sprintf(s, "%d", hour);
//t = sdlx_create_text_texture(s);
//#define CLOCK_CENTER_X  500
//#define CLOCK_CENTER_Y  600

int angle = 30;
#define EVID_XXX 1

// -----------------  MAIN  ------------------------------------------

int main(int argc, char **argv)
{
    sdlx_event_t    event;
    int             rc;
    bool            quit = false;

    // save arg values
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);
    printf("INFO %s: sdlx_win_width/height  = %d %d\n", progname, sdlx_win_width, sdlx_win_height);

    // init sdl video subsystem
    rc = sdlx_init(SUBSYS_VIDEO);
    if (rc != 0) {
        printf("ERROR %s: sdlx_init failed\n", progname);
        return 1;
    }

    // 

    // runtime loop
    while (!quit) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // register control event to end program
        sdlx_register_control_events(">", NULL, "X", COLOR_BLACK, EVID_XXX, 0, EVID_QUIT);

        // draw the analog clock
        draw_analog_clock_numbers();
        draw_analog_clock_hands();

        // present the display
        sdlx_display_present();

        // wait for an event with 50 ms timeout;
        // if no event, then redraw display
        sdlx_get_event(50000, &event);
        if (event.event_id == -1) {
            continue;
        }

        // process events
        switch (event.event_id) {
        case EVID_XXX:
            angle += 30;
            break;
        case EVID_QUIT:
            quit = true;
            break;
        }
    }

    // cleanup and end program
    sdlx_quit(SUBSYS_VIDEO);
    printf("INFO %s: terminating\n", progname);
    return 0;
}

sdlx_texture_t *hour_hand;
sdlx_texture_t *minute_hand;
sdlx_texture_t *second_hand;

static void draw_analog_clock_numbers(void)
{
    int hour, x, y;

    sdlx_render_fill_rect(0, 100, 1000, 1000, COLOR_WHITE);

    sdlx_print_init(DEFAULT_FONT, COLOR_BLACK, COLOR_WHITE);

    for (hour = 1; hour <= 12; hour++) {
        x = 500 + 400 * sin(hour * 30 * (M_PI / 180));  //xxx use deines
        y = 600 - 400 * cos(hour * 30 * (M_PI / 180));
        sdlx_render_printf_xyctr(x, y, "%d", hour);
    }
}

#define X_CLOCK 500
#define Y_CLOCK 600

#define W_HH  27
#define H_HH  300
#define O_HH  40

#define W_MH  17
#define H_MH  375
#define O_MH  60

#define W_SH  7
#define H_SH  470
#define O_SH  120

static void draw_analog_clock_hands(void)
{
    static bool first_call = true;

    if (first_call) {
        hour_hand = create_rectangle_texture(W_HH, H_HH, COLOR_BLACK);
        minute_hand = create_rectangle_texture(W_MH, H_MH, COLOR_BLACK);  // xxx free these
        second_hand = create_rectangle_texture(W_SH, H_SH, COLOR_RED);  // xxx free these
        first_call = false;
    }

    sdlx_render_texture_ex(X_CLOCK-(W_HH/2), Y_CLOCK-H_HH+O_HH, 
                           W_HH, H_HH, 
                           angle, 
                           W_HH/2, H_HH-O_HH,
                           hour_hand);

    sdlx_render_texture_ex(X_CLOCK-(W_MH/2), Y_CLOCK-H_MH+O_MH, 
                           W_MH, H_MH, 
                           angle+90, 
                           W_MH/2, H_MH-O_MH,
                           minute_hand);

    sdlx_render_texture_ex(X_CLOCK-(W_SH/2), Y_CLOCK-H_SH+O_SH, 
                           W_SH, H_SH, 
                           angle+150,
                           W_SH/2, H_SH-O_SH,
                           second_hand);

    sdlx_render_point(X_CLOCK, Y_CLOCK, COLOR_RED, 9);
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
