#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include <sdlx.h>
#include <utils.h>

// xxx
// - smooth result values
// - handle other orientations
// - param for tilt limit

// defines
#define RAD_TO_DEG (180 / M_PI)
#define DEG_TO_RAD (M_PI / 180)

// variables
char *progname;
char *data_dir;

sdlx_texture_t *green_circle;
sdlx_texture_t *blue_circle;
sdlx_texture_t *red_circle;
sdlx_texture_t *gray_circle;
sdlx_texture_t *light_gray_circle;
    
// prototypes
void smooth(double newval, double *smoothed);
void horizontal(double ax, double ay, double az);

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

    // create textures
    green_circle      = sdlx_create_filled_circle_texture(50, COLOR_GREEN);
    blue_circle       = sdlx_create_filled_circle_texture(50, COLOR_BLUE);
    red_circle        = sdlx_create_filled_circle_texture(50, COLOR_RED);
    gray_circle       = sdlx_create_filled_circle_texture(500, COLOR_GRAY);
    light_gray_circle = sdlx_create_filled_circle_texture(500, COLOR_LIGHT_GRAY);

    // use default font size and color
    sdlx_print_init(DEFAULT_FONT, COLOR_WHITE, COLOR_BLACK);

    // runtime loop
    while (!done) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // register control event to
        // - end program
        sdlx_register_control_events(NULL, NULL, "X", COLOR_WHITE, COLOR_BLACK, 0, 0, EVID_QUIT);

        // read and smooth accelerometer values
        rc = sdlx_sensor_read_accelerometer(&ax_raw, &ay_raw, &az_raw);
        if (rc != 0) {
            sdlx_render_printf_xyctr(sdlx_win_width/2, sdlx_win_height/2, "No Accelerometer");
            goto display_present;
        }
        smooth(ax_raw, &ax);
        smooth(ay_raw, &ay);
        smooth(az_raw, &az);

        // if orientation is horizontal
        if (1) {
            horizontal(ax, ay, az);
        }

        // present the display
display_present:
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

    // free allocations
    sdlx_destroy_texture(green_circle);
    sdlx_destroy_texture(blue_circle);
    sdlx_destroy_texture(red_circle);
    sdlx_destroy_texture(gray_circle);
    sdlx_destroy_texture(light_gray_circle);

    // quit sdl subsystems and end program
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

    *smoothed = K * *smoothed + (1.0 - K) * newval;
}

// -----------------  HORIZONTAL  ---------------------------

void horizontal(double ax, double ay, double az)
{
    int             width, xctr, yctr;
    sdlx_texture_t *t;
    double          tilt_dir, tilt_amount;
    int             max_bulls_eye, deg;

    // set the max bulls-eye tilt amount (degrees)
    max_bulls_eye = 5;  // xxx adjst

    // init center location of the bulls-eye
    xctr = sdlx_win_width/2;
    yctr = sdlx_win_height/2;

    // draw bulls-eye
    t = gray_circle;
    for (deg = max_bulls_eye; deg >= 1; deg--) {
        width = nearbyint((double)sdlx_win_width / max_bulls_eye * deg);
        t = (t == gray_circle ? light_gray_circle : gray_circle);
        sdlx_render_texture(xctr-width/2, yctr-width/2, width, width, t);
    }



//  t = gray_circle;
//  for (width = sdlx_win_width; width >= 100; width -= 100) {
//      t = (t == gray_circle ? light_gray_circle : gray_circle);
//      sdlx_render_texture(xctr-width/2, yctr-width/2, width, width, t);
//  }


    // calculate tilt amount and direction
    tilt_dir    = atan2(ax, ay) * RAD_TO_DEG;
    tilt_amount = atan( sqrt(ax*ax + ay*ay) / az ) * RAD_TO_DEG;

    tilt_dir = 90;
    tilt_amount = 5;

    // limit tilt amount to the max that can be displayed on the bulls-eye
    if (tilt_amount > max_bulls_eye) {
        tilt_amount = max_bulls_eye;
    }
    
    // print results
    sdlx_render_printf(0, ROW2Y(1), "axyz % 4.1f % 4.1f % 4.1f", ax, ay, az);
    sdlx_render_printf(0, ROW2Y(3), "tilt %4.2f @ %3.0f deg", tilt_amount, tilt_dir);

    // display small circle on the bulls-eye pattern, 
    // at location indicating the tilt direction and amount
    double dx, dy;
    int x, y, small_circle_diameter;

    dx = tilt_amount * sin(tilt_dir*DEG_TO_RAD) * ((double)(sdlx_win_width/2) / max_bulls_eye);
    dy = tilt_amount * cos(tilt_dir*DEG_TO_RAD) * ((double)(sdlx_win_width/2) / max_bulls_eye);
    x = nearbyint(xctr  + dx);
    y = nearbyint(yctr - dy);

    small_circle_diameter = 50;

    t = ((fabs(tilt_amount) < 0.1)            ? green_circle :
         ((fabs(tilt_amount) < max_bulls_eye) ? blue_circle :
                                                red_circle));

    sdlx_render_texture(x-small_circle_diameter/2, 
                        y-small_circle_diameter/2, 
                        small_circle_diameter, 
                        small_circle_diameter, 
                        t);
}
