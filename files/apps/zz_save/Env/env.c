#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <math.h>

#include <sdlx.h>
#include <utils.h>

#include "svcs/Sensors/sensors.h"

// xxx todo
// - full review, and comments
// - multi plots
// - plot points, not bars for hourly plot
// - end selector control
// - bar graph
// - hour vs day select
// - settings
// - clip graph
// - at bottom of display, or on other display,  display the current values
// - set EOD color to Green if at EOD
// - review how switching between daily and hourly graphs work
// - add 31 day graph interval too
// - make routine to init the plots, and use this also to update which plots are shown;
//   OR perhaps this program just shows fixed set of plots
// - only plot if data is valid;  or display NO DATA within the plot if there are no points

//
// defines
//

#define MAX_PTS 100

#define MAX_PLOTS 3

#define NUM_DAYS 7

#define EVID_EOD          1
#define EVID_DISPLAY_MODE 2

#define MAX_DISPLAY_MODE 2
#define DISPLAY_MODE_HOURLY 0
#define DISPLAY_MODE_DAILY  1

//
// typedefs
//

typedef struct {
    int min_pressure;
    int max_pressure;
} params_t;

typedef struct {
    char  *title;
    int    which;
    int    ybottom;
    int    ytop;
    double yval_bottom;
    double yval_top;
    double yval_of_x_axis;
} plot_t;

//
// variables
//

char           *progname;
plot_t          plots[MAX_PLOTS];
int             display_mode = DISPLAY_MODE_HOURLY;
sensors_data_t *data;

//
// prototypes
//

int start_hour_of_tomorrow(void);

void plot_hourly(plot_t *p, int psh, int peh, int ybottom, int ytop);
void get_hourly_plot_pts(int which, int psh, int peh, sdlx_plot_point_t *pts, int *num_pts);

void plot_daily(plot_t *p, int psh, int peh, int ybottom, int ytop);
void get_daily_plot_pts(
            int which, int psh, int peh,
            sdlx_plot_point_t *pts_avg, sdlx_plot_point_t * pts_min, sdlx_plot_point_t * pts_max,
            int *num_pts);

// -----------------  MAIN  ------------------------------------------

int main(int argc, char **argv)
{
    int             rc, i;
    sdlx_event_t     event;
    bool            end_program = false;
    int             psh, peh;
    double          peh_float;
    sdlx_loc_t      *loc;
    char           *str;

    // save args
    progname = argv[0];

    // print startup messages
    printf("INFO %s: starting\n", progname);
    printf("INFO %s: sensors.dat:\n", progname);
    printf("INFO %s:   version supported = %lx\n", progname, SENSORS_DATA_FILE_VERSION);
    printf("INFO %s:   size              = %zd\n", progname, sizeof(sensors_data_t));

    // define plots
    plots[0].title          = "Pressure";
    plots[0].which          = ASENSOR_PRESSURE;
    plots[0].yval_bottom    = 950;
    plots[0].yval_top       = 1050;
    plots[0].yval_of_x_axis = 1000;

    plots[1].title          = "Temperature";
    plots[1].which          = WEATHER_GOV_TEMPERATURE;
    plots[1].yval_bottom    = 0;
    plots[1].yval_top       = 100;
    plots[1].yval_of_x_axis = INVALID_NUMBER;

    // map the sensors.dat file;
    // if map failed or file version is incorrect then return error
    data = util_map_file("svcs/Sensors", "sensors.dat", sizeof(sensors_data_t), false);
    if (data == NULL) {
        printf("ERROR %s: failed to map sensors.dat\n", progname);
        return 1;
    }
    if (data->hdr.version != SENSORS_DATA_FILE_VERSION) {
        printf("ERROR %s: sensors.dat version %lx is not supported, expected %lx\n",
               progname, data->hdr.version, SENSORS_DATA_FILE_VERSION);
        return 1;
    }
    printf("INFO %s: sensors.dat mapped and version verified\n", progname);

    // init sdl
    rc = sdlx_init(SUBSYS_VIDEO);
    if (rc != 0) {
        printf("ERROR %s: sdlx_init failed\n", progname);
        return 1;
    }

    // xxx
    peh_float = start_hour_of_tomorrow();

    // runtime loop
    while (true) {
        // init the backbuffer, and printing
        sdlx_display_init(COLOR_BLACK);
        sdlx_print_init(DEFAULT_FONT, COLOR_WHITE, COLOR_BLACK);

        // register control events
        // 'X' - end prorgram
        sdlx_register_control_events(NULL, NULL, "X", 
                                    COLOR_BLACK,
                                    0, 0, EVID_QUIT);
        sdlx_register_event(NULL, EVID_MOTION);

        // xxx
        sdlx_print_init_color(COLOR_LIGHT_BLUE, COLOR_BLACK);

        loc = sdlx_render_printf(sdlx_win_width-3*sdlx_char_width, sdlx_win_height-200, "%s", "EOD");
        sdlx_register_event(loc, EVID_EOD);

        str = (display_mode == DISPLAY_MODE_DAILY ? "DAILY" : "HOURLY");
        loc = sdlx_render_printf(sdlx_win_width/2-strlen(str)*sdlx_char_width/2, 
                                sdlx_win_height-200, "%s", str);
        sdlx_register_event(loc, EVID_DISPLAY_MODE);

        sdlx_print_init_color(COLOR_WHITE, COLOR_BLACK);

        // xxx comment
        if (display_mode == DISPLAY_MODE_DAILY) {
            time_t t;
            struct tm tm;

            peh = nearbyint(peh_float);

            t = peh * 3600;
            localtime_r(&t, &tm);
            if (tm.tm_hour != 0) {
                peh = peh - tm.tm_hour + 24;
            }

            psh = peh - NUM_DAYS*24;
        } else {
            peh = nearbyint(peh_float);
            psh = peh - 24;
        }

        // xxx
        for (i = 0; i < MAX_PLOTS; i++) {
            if (plots[i].title == NULL) {
                continue;
            }

            // xxx fix
            int ybottom = 600 * (i + 1);
            int ytop    = ybottom - 600;
            if (display_mode == DISPLAY_MODE_HOURLY) {
                plot_hourly(&plots[i], psh, peh, ybottom, ytop);
            } else {
                plot_daily(&plots[i], psh, peh, ybottom, ytop);
            }
        }

        // present the display
        sdlx_display_present();

        // wait for an event with 500 ms timeout;
        // if no event available, then redraw display
        sdlx_get_event(500000, &event);
        if (event.event_id == -1) {
            continue;
        }

        // process events
        switch (event.event_id) {
        case EVID_QUIT:
            end_program = true;
            break;      
        case EVID_MOTION: {
            if (display_mode == DISPLAY_MODE_HOURLY) {
                peh_float -= event.u.motion.xrel * (24. / sdlx_win_width);
            } else {
                peh_float -= event.u.motion.xrel * ((24. * NUM_DAYS) / sdlx_win_width);
            }
            int xxx = start_hour_of_tomorrow();
            if (peh_float > xxx) {
                printf("XXX limitting peh_float\n");
                peh_float = xxx;
            }
            break;       }
        case EVID_EOD:
            printf("INFO %s: got EVID_EOD\n", progname);
            peh_float = start_hour_of_tomorrow();
            break;
        case EVID_DISPLAY_MODE: {
            printf("INFO %s: got EVID_DISPLAY_MODE\n", progname);

            double ctr_hour = (peh + psh) / 2.;
            display_mode = (display_mode + 1 == MAX_DISPLAY_MODE ? 0 : display_mode + 1);

            int xxx = start_hour_of_tomorrow();
            if (peh_float == xxx) {
                break;
            }

            if (display_mode == DISPLAY_MODE_HOURLY) {
                peh_float = ctr_hour + 12;
            } else {
                peh_float = ctr_hour + (NUM_DAYS / 2.) * 24;
            }

            if (peh_float > xxx) {
                printf("XXX limitting peh_float\n");
                peh_float = xxx;
            }

            break; }
        }

        // if end_program flag is set then break out of runtime loop
        if (end_program) {
            break;
        }
    }

    // cleanup and end program
    sdlx_quit(SUBSYS_VIDEO);
    util_unmap_file(data);
    printf("INFO %s: terminating\n", progname);
    return 0;
}

int start_hour_of_tomorrow(void)
{
    struct tm tm;
    time_t t = time(NULL);
    localtime_r(&t, &tm);
    return t / 3600 - tm.tm_hour + 24;
}

// -----------------  PLOT DAILY  -------------------------

void plot_daily(plot_t *p, int psh, int peh, int ybottom, int ytop)
{
    sdlx_plot_point_t pts_avg[MAX_PTS], pts_min[MAX_PTS], pts_max[MAX_PTS];
    void            *cx;
    int              num_pts;
    char             xmin_str[50], xmax_str[50], ymin_str[50], ymax_str[50];
    time_t           t;
    struct tm        tm;

    // get point values for the plot start hour to plot end hour (psh - peh) range
    get_daily_plot_pts(p->which, psh, peh, pts_avg, pts_min, pts_max, &num_pts);

    // create the plot context
    cx =  sdlx_plot_create(p->title,                    // title
                          0, sdlx_win_width-1,          // xleft, xright
                          ybottom, ytop,               // ybottom, ytop
                          psh, peh,                    // xval_left, xval_right
                          p->yval_bottom, p->yval_top, // yval_bottom, yval_top
                          p->yval_of_x_axis);          // yval_of_x_axis

    // plot the data points, using bar graph
    sdlx_plot_bars(cx, pts_avg, pts_min, pts_max, num_pts, 24);

    // init strings for the plot x/y-axis labels
    t = psh * 3600;
    localtime_r(&t, &tm);
    sprintf(xmin_str, "%02d/%02d/%02d", tm.tm_mon+1, tm.tm_mday, tm.tm_year-100);
    t = peh * 3600 - 1;
    localtime_r(&t, &tm);
    sprintf(xmax_str, "%02d/%02d/%02d", tm.tm_mon+1, tm.tm_mday, tm.tm_year-100);

    sprintf(ymin_str, "%.0f", p->yval_bottom);
    sprintf(ymax_str, "%.0f", p->yval_top);

    // plot the axes
    sdlx_plot_axis(cx, xmin_str, xmax_str, ymin_str, ymax_str);

    // free the plot
    sdlx_plot_free(cx);
}

void get_daily_plot_pts(
            int which, int psh, int peh,
            sdlx_plot_point_t *pts_avg, sdlx_plot_point_t * pts_min, sdlx_plot_point_t * pts_max,
            int *num_pts)
{
    int    day_start_hour, hour, idx, k, n=0;
    double value, sum, min, max;

    for (day_start_hour = psh; day_start_hour < peh; day_start_hour += 24) {
        sum = k = 0;
        min = 1e99;;
        max = -1e99;;
        for (hour = day_start_hour; hour < day_start_hour+24; hour++) {
            idx = hour - data->hdr.start_hour;
            if (idx < 0 || idx > data->hdr.last_idx || idx >= MAX_SENSOR_VALUES) {
                continue;
            }

            value = data->values[idx].sensors[which];
            if (value == INVALID_NUMBER) {
                continue;
            }

            sum += value;
            k++;

            if (value < min) min = value;
            if (value > max) max = value;
        }

        if (k == 0) {
            continue;
        }

        pts_avg[n].xval = day_start_hour + 12;
        pts_avg[n].yval = sum / k;
        pts_min[n].xval = day_start_hour + 12;
        pts_min[n].yval = min;
        pts_max[n].xval = day_start_hour + 12;
        pts_max[n].yval = max;
        n++;
    }

    *num_pts = n;
}

// -----------------  PLOT HOURLY  --------------------------

void plot_hourly(plot_t *p, int psh, int peh, int ybottom, int ytop)
{
    sdlx_plot_point_t    pts[MAX_PTS];
    void           *cx;
    int             num_pts;
    char            xmin_str[50], xmax_str[50], ymin_str[50], ymax_str[50];
    time_t          t;
    struct tm       tm;

    // get point values for the plot start hour to plot end hour (psh - peh) range
    get_hourly_plot_pts(p->which, psh, peh, pts, &num_pts);

    // create the plot context
    cx =  sdlx_plot_create(p->title,                    // title
                          0, sdlx_win_width-1,          // xleft, xright
                          ybottom, ytop,               // ybottom, ytop
                          //xxx psh, psh+NUM_DAYS*24,        // xval_left, xval_right
                          psh, peh,        // xval_left, xval_right
                          p->yval_bottom, p->yval_top, // yval_bottom, yval_top
                          p->yval_of_x_axis);          // yval_of_x_axis

    // plot the data points, using bar graph
    sdlx_plot_bars(cx, pts, pts, pts, num_pts, 1);

    // init strings for the plot x/y-axis labels
    t = psh * 3600;
    localtime_r(&t, &tm);
    sprintf(xmin_str, "%02d/%02d/%02d %02d", tm.tm_mon+1, tm.tm_mday, tm.tm_year-100, tm.tm_hour);
    t = peh * 3600 - 1;
    localtime_r(&t, &tm);
    sprintf(xmax_str, "%02d/%02d/%02d %02d", tm.tm_mon+1, tm.tm_mday, tm.tm_year-100, tm.tm_hour);

    sprintf(ymin_str, "%.0f", p->yval_bottom);
    sprintf(ymax_str, "%.0f", p->yval_top);

    // plot the axes
    sdlx_plot_axis(cx, xmin_str, xmax_str, ymin_str, ymax_str);

    // free the plot
    sdlx_plot_free(cx);
}

void get_hourly_plot_pts(int which, int psh, int peh, sdlx_plot_point_t *pts, int *num_pts)
{
    int hour, idx, n=0;
    double value;

    for (hour = psh; hour < peh; hour++) {
        idx = hour - data->hdr.start_hour;
        if (idx < 0 || idx > data->hdr.last_idx || idx >= MAX_SENSOR_VALUES) {
            continue;
        }

        value = data->values[idx].sensors[which];
        if (value == INVALID_NUMBER) {
            continue;
        }

        pts[n].xval = hour + 0.5;
        pts[n].yval = value;
        n++;

        // xxx limit value to min/max
    }

    *num_pts = n;
}
