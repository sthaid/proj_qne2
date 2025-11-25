#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include <sdlx.h>
#include <utils.h>

// defines
#define RAD_TO_DEG (180 / M_PI)
#define DEG_TO_RAD (M_PI / 180)

// variables
char *progname;
char *data_dir;
    
// prototypes
void smooth(double newval, double *smoothed);

// -----------------  MAIN  ------------------------------------------
    
int main(int argc, char **argv)
{
    int          rc;
    sdlx_event_t event;
    bool         done = false;
    double       ax_raw, ay_raw, az_raw;
    double       ax=INVALID_NUMBER, ay=INVALID_NUMBER, az=INVALID_NUMBER;

    // save args
    if (argc != 2) {
        printf("ERROR: data_dir arg expected\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    // init sdl video subsystem
    rc = sdlx_init(SUBSYS_VIDEO|SUBSYS_SENSOR);
    if (rc != 0) {
        printf("ERROR %s: sdlx_init failed\n", progname);
        return 1;
    }

    sdlx_texture_t *
    green_circle = sdlx_create_filled_circle_texture(100, COLOR_GREEN);

    // init font size and color
    sdlx_print_init(DEFAULT_FONT, COLOR_WHITE, COLOR_BLACK);

    // runtime loop
    while (!done) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // register control event to
        // - end program
        sdlx_register_control_events(NULL, NULL, "X", COLOR_WHITE, COLOR_BLACK, 0, 0, EVID_QUIT);

        // read and display accelerometer values
        rc = sdlx_sensor_read_accelerometer(&ax_raw, &ay_raw, &az_raw);
        if (rc != 0) {
            //sdlx_render_printf_xyctr(sdlx_win_width/2, sdlx_win_height/2, "No Accelerometer");
            //goto display_present;
            ax_raw = 0.1;
            ay_raw = 0.1;
            az_raw = 9.8;
        }

        smooth(ax_raw, &ax);
        smooth(ay_raw, &ay);
        smooth(az_raw, &az);

// xxx
// - smooth values
// - handle other orientations
// - display tilt amount, and rotat for different orientations
// - display a cirle instead, color green when close to no tilt
// - param for tilt limit
// - limit tilt to circular area,  and display this area in gray
// - display target circle, which is for the tilt limit
// - debug display mode


        sdlx_render_printf(0, ROW2Y(1), "ax = %5.2f", ax);
        sdlx_render_printf(0, ROW2Y(2), "ay = %5.2f", ay);
        sdlx_render_printf(0, ROW2Y(3), "az = %5.2f", az);

        // xxx
        double tilt_dir, tilt_amount;
        tilt_dir   = atan2(ax, ay) * RAD_TO_DEG;
        tilt_amount = atan( sqrt(ax*ax + ay*ay) / az ) * RAD_TO_DEG;
        sdlx_render_printf(0, ROW2Y(5), "tilt_dir   = %0.2f", tilt_dir);
        sdlx_render_printf(0, ROW2Y(6), "tilt_amount = %0.2f", tilt_amount);

        double dx, dy, scale;
        int x, y;

        scale = sdlx_win_width/2 / 10.0;

        dx = tilt_amount * sin(tilt_dir*DEG_TO_RAD) * scale;
        dy = tilt_amount * cos(tilt_dir*DEG_TO_RAD) * scale;

        x = nearbyint(sdlx_win_width/2  + dx);
        y = nearbyint(sdlx_win_height/2 - dy);

        int wh = 100;
        sdlx_render_texture(x-wh/2, y-wh/2, wh, wh, green_circle);
        //sdlx_render_printf_xyctr(x, y, "X");

        // present the display
//display_present:
        sdlx_display_present();

        // wait for event, with 100ms timeout
        sdlx_get_event(100000, &event);

        // process events
        switch (event.event_id) {
        case EVID_QUIT:
            done = true;
            break;
        }
    }

    sdlx_destroy_texture(green_circle);

    // cleanup and end program
    sdlx_quit(SUBSYS_VIDEO|SUBSYS_SENSOR);
    printf("INFO %s: terminating\n", progname);
    return 0;
}

#define K 0.9

void smooth(double newval, double *smoothed)
{
    if (*smoothed == INVALID_NUMBER) {
        *smoothed = newval;
        return;
    }

    double delta = newval - *smoothed;
    *smoothed = K * *smoothed + (1.0 - K) * newval;
}

