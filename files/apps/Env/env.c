#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>  // for memset

#include <sdl.h>
#include <utils.h>

#include "svcs/Sensors/sensors.h"

// xxx
// - use ints for sensors, and leave spares
// - buy wifi temperature sensor
// - check into auto start   SampleForegroundService
// - store time_t in sensors.dat, in addition to what is there

// xxx temp
typedef struct {
    double xval;
    double yval;
} plot_point_t;
void *sdl_plot_create(char *title, 
                      int xleft, int xright, int ybottom, int ytop,
                      double xval_min, int xval_max, double yval_min, int yval_max,
                      double yval_of_x_axis);
void sdl_plot_points(void *cx_arg, plot_point_t *p_arg, int n_arg);
void sdl_plot_free(void *cx);

//
// defines
//

#define SECS_PER_HOUR 3600 //xxx

//
// typedefs
//

typedef struct {
    int min_pressure;
    int max_pressure;
} params_t;

//
// variables
//

static char *progname;
static char *data_dir;

//
// prototypes
//

char *sensval2str(double x);

// -----------------  MAIN  ------------------------------------------

int main(int argc, char **argv)
{
    int             rc;
    sdl_event_t     event;
    bool            end_program = false;
    sensors_data_t *data;
    double          motion = 0;

    int i,j;

    // save args
    progname = argv[0];
    data_dir = argv[1];

    // print startup messages
    printf("INFO %s: starting, data_dir = %s\n", progname, data_dir);
    printf("INFO %s: sensors.dat:\n", progname);
    printf("INFO %s:   version supported = %lx\n", progname, SENSORS_DATA_FILE_VERSION);
    printf("INFO %s:   size              = %zd\n", progname, sizeof(sensors_data_t));

    // get params
    util_get_int_param(data_dir, "min_pressure", 950);
    util_get_int_param(data_dir, "max_pressure", 1050);

    // map the sensors.dat file;
    // if map failed or file version is incorrect then return error
    data = util_map_file("svcs_data/Sensors", "sensors.dat", sizeof(sensors_data_t), false);
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
    rc = sdl_init(SUBSYS_VIDEO);
    if (rc != 0) {
        printf("ERROR %s: sdl_init failed\n", progname);
        return 1;
    }

    while (true) {
        // init the backbuffer, and print font & color
        sdl_display_init(COLOR_BLACK);
        sdl_print_init(DEFAULT_FONT, COLOR_WHITE, COLOR_BLACK);

        // register control events
        // 'X' - end prorgram
        sdl_register_control_events(NULL, NULL, "X", 
                                    COLOR_BLACK,
                                    0, 0, EVID_QUIT);
        sdl_register_event(NULL, EVID_MOTION);

        // xxx
        // - change to span
        // - dont use full win_width span, use one less
        void *cx;
        plot_point_t pts[100];
        for (i = 0; i < 100; i++) {
            pts[i].xval = 0.5 + i;
            pts[i].yval = 950 + i;
        }

        cx =  sdl_plot_create("TITLE", 0, sdl_win_width-1, 800, 100,
                              motion, motion+24, 950, 1050, 1000);

        for (i = 0; i < 100; i++) {
            if (pts[i].xval > motion) {
                break;
            }
        }
        for (j = i; j < 100; j++) {
            if (pts[j].xval > motion+24) {
                break;
            }
        }

        sdl_plot_points(cx, &pts[i], j-i);

        sdl_plot_free(cx);

        // register xxx events
        //sdl_register_event(NULL, EVID_MOTION);
//      idx = data->hdr.next-1;
//      printf("INFO %s: idx %d\n", progname, idx);
//      plot(idx,
//           params.min_pressure,
//           params.max_pressure,
//           DURATION_WEEK,
//           PRESSURE);

        // present the display
        sdl_display_present();

        // wait for an event with 50 ms timeout;
        // if no event available, then redraw display
        sdl_get_event(50000, &event);
        if (event.event_id == -1) {
            continue;
        }

        // process events
        switch (event.event_id) {
        case EVID_QUIT:
            end_program = true;
            break;      
        case EVID_MOTION:
            motion -= event.u.motion.xrel * (24. / sdl_win_width);
            break;      
        }

        if (end_program) {
            break;
        }
    }

    // exit sdl
    sdl_quit(SUBSYS_VIDEO);

    util_unmap_file(data);

    // return success
    printf("INFO %s: terminating\n", progname);
    return 0;
}

#define MAX_STR_TBL 8
char *sensval2str(double x)
{
    static char str_tbl[MAX_STR_TBL][50];
    static int  n;
    char *s;

    if (x == INVALID_SENSOR_VALUE) {
        return "invld";
    } else {
        s = str_tbl[n];
        n = (n + 1) % MAX_STR_TBL;
        sprintf(s, "%.0f", x);
        return s;
    }
}

// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char   title[64];
    int    xleft;
    int    xright;
    int    xspan;
    int    ybottom;
    int    ytop;
    int    yspan;
    double xval_min;
    double xval_max;
    double yval_min;
    double yval_max;
    double yval_of_x_axis;
} plot_cx_t;  // private


int xval2x(plot_cx_t *cx, double xval)
{
    int x;

// xxx yval_span
    x = nearbyint(cx->xleft + (xval - cx->xval_min) * cx->xspan / (cx->xval_max - cx->xval_min));
    if (x < cx->xleft) x = cx->xleft;
    if (x > cx->xright) x = cx->xright;

    return x;
}

int yval2y(plot_cx_t *cx, double yval)
{
    int y;

// xxx yval_span
    y = nearbyint(cx->ybottom - (yval - cx->yval_min) * cx->yspan / (cx->yval_max - cx->yval_min));
    if (y > cx->ybottom) y = cx->ybottom;
    if (y < cx->ytop) y = cx->ytop;

    return y;
}

void *sdl_plot_create(char *title, 
                      int xleft, int xright, int ybottom, int ytop,
                      double xval_min, int xval_max, double yval_min, int yval_max,
                      double yval_of_x_axis)
{
    plot_cx_t *cx;
    int i;

    // alloc cx and save params in cx
    cx = calloc(1, sizeof(plot_cx_t));
    strcpy(cx->title, title);
    cx->xleft          = xleft;
    cx->xright         = xright;
    cx->xspan          = xright - xleft + 1;
    cx->ybottom        = ybottom;
    cx->ytop           = ytop;
    cx->yspan          = ybottom - ytop + 1;
    cx->xval_min       = xval_min;
    cx->xval_max       = xval_max;
    cx->yval_min       = yval_min;
    cx->yval_max       = yval_max;
    cx->yval_of_x_axis = yval_of_x_axis;

    // draw y-axis on both left and right
    for (i = 0; i < 3; i++) {
        sdl_render_line(xleft+i, ybottom, xleft+i, ytop, COLOR_BLUE);
        sdl_render_line(xright-i, ybottom, xright-i, ytop, COLOR_BLUE);
    }

    // draw x-axis
    int y = yval2y(cx, yval_of_x_axis);
    for (i = -1; i <= 1; i++) {
        sdl_render_line(xleft, y+i, xright, y+i, COLOR_BLUE);
    }

    // label x and y axis
    // xxx

    // return cx
    return cx;
}

void sdl_plot_points(void *cx_arg, plot_point_t *p_arg, int n_arg)
{
    plot_cx_t *cx = (plot_cx_t*)cx_arg;
    sdl_point_t *points;
    int i, n=0, point_size=5;

    points = malloc(n_arg * sizeof(sdl_point_t));

    for (i = 0; i < n_arg; i++) {
        points[n].x = xval2x(cx, p_arg[i].xval);
        points[n].y = yval2y(cx, p_arg[i].yval);
        n++;
    }
    sdl_render_points(points, n, COLOR_WHITE, point_size);

    free(points);
}

//void sdl_plot_bars(plot_bar_t *b, int n)
//{
//}


void sdl_plot_free(void *cx)
{
    // free cx
    free(cx);
}

