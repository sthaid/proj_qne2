#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>  // for memset
#include <unistd.h>   // xxx check these are needed

#include <sdl.h>
#include <utils.h>

#include "svcs/Sensors/sensors.h"

// xxx
// - use ints for sensors, and leave spares
// - buy wifi temperature sensor
// - check into auto start   SampleForegroundService
// - store time_t in sensors.dat, in addition to what is there

// xxx todo
// - multi plots
// - end selector control
// - bar graph
// - hour vs day select
// - settings
// - clip graph
// - start daily plot on day interval,  < > go by days

// xxx temp
typedef struct {
    double xval;
    double yval;
} plot_point_t;
void *sdl_plot_create(char *title, 
                      int xleft, int xright, int ybottom, int ytop,
                      double xval_left, int xval_right, double yval_bottom, int yval_top,
                      double yval_of_x_axis);
void sdl_plot_points(void *cx_arg, plot_point_t *pts, int num_pts);
void sdl_plot_bars(void *cx_arg, 
                   plot_point_t *pts_avg, plot_point_t *pts_min, plot_point_t *pts_max,
                   int num_pts, double bar_wval);
void sdl_plot_free(void *cx);

//
// defines
//

#define MAX_PTS 100

#define MAX_PLOTS 3

#define NUM_DAYS 10

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
char           *data_dir;
sensors_data_t *data;

bool hourly = false; //xxx

plot_t plots[MAX_PLOTS];

//
// prototypes
//

void get_plot_pts(int which, int psh, int peh, plot_point_t *pts, int *num_pts);

void plot_daily(plot_t *p, int psh, int peh, int ybottom, int ytop);

void get_plot_day_pts(
            int which, int psh, int peh,
            plot_point_t *pts_avg, plot_point_t * pts_min, plot_point_t * pts_max,
            int *num_pts);

// -----------------  MAIN  ------------------------------------------

int main(int argc, char **argv)
{
    int             rc;
    sdl_event_t     event;
    bool            end_program = false;
    time_t          plot_end_time;
    struct tm       plot_end_tm;
    int             psh, peh;

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

    // define plots
    plots[0].title          = "Pressure";
    plots[0].which          = PRESSURE;
    plots[0].yval_bottom    = util_get_int_param(data_dir, "min_pressure", 950);  //xxx cleanup
    plots[0].yval_top       = util_get_int_param(data_dir, "max_pressure", 1050);
    plots[0].yval_of_x_axis = util_get_int_param(data_dir, "typical_pressure", 1000);  // xxx 1013

//  plots[1].title          = "Pressure";
//  plots[1].which          = PRESSURE;
//  plots[1].yval_bottom    = util_get_int_param(data_dir, "min_pressure", 950);  //xxx cleanup
//  plots[1].yval_top       = util_get_int_param(data_dir, "max_pressure", 1050);
//  plots[1].yval_of_x_axis = util_get_int_param(data_dir, "typical_pressure", 1000);  // xxx 1013

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

    // xxx
    plot_end_time = time(NULL);

    // runtime loop
    while (true) {
        // init the backbuffer, and printing
        sdl_display_init(COLOR_BLACK);
        sdl_print_init(DEFAULT_FONT, COLOR_WHITE, COLOR_BLACK);

        // register control events
        // 'X' - end prorgram
        sdl_register_control_events(NULL, NULL, "X", 
                                    COLOR_BLACK,
                                    0, 0, EVID_QUIT);
        sdl_register_event(NULL, EVID_MOTION);


        if (hourly) {
#if 0
            // print the plot end hour 
            localtime_r(&plot_end_time, &plot_end_tm);
            sdl_render_printf_xyctr(
                    sdl_win_width/2, sdl_win_height-200, 
                    "%02d/%02d/%02d %02d",
                    plot_end_tm.tm_mon+1, plot_end_tm.tm_mday, plot_end_tm.tm_year-100, plot_end_tm.tm_hour);

            // xxx these are utc
            peh = plot_end_time / 3600;  // plot end hour
            psh = peh - 23;              // plot start hour  xxx make hourly for the current day

            // xxx
            get_plot_pts(PRESSURE, psh, peh, pts_avg, &num_pts);
            plot_cx =  sdl_plot_create("PRESSURE", 
                                       0, sdl_win_width-1,    // xleft, xright
                                       800, 0,              // ybottom, ytop
                                       psh, psh+24,         // xval_left, xval_right
                                       950, 1050            // yval_bottom, yval_top
                                       1000);               // yval_of_x_axis
            sdl_plot_points(plot_cx, pts_avg, num_pts);
            sdl_plot_free(plot_cx);
#endif
        } else {  // daily
            // print the plot end day
            localtime_r(&plot_end_time, &plot_end_tm);
            sdl_render_printf_xyctr(
                    sdl_win_width/2, sdl_win_height-200, 
                    "%02d/%02d/%02d",
                    plot_end_tm.tm_mon+1, plot_end_tm.tm_mday, plot_end_tm.tm_year-100);

            // xxx these are utc
            peh = plot_end_time / 3600 - plot_end_tm.tm_hour;
            psh = peh - ((NUM_DAYS-1) * 24);

            // xxx
            for (int i = 0; i < MAX_PLOTS; i++) {
                if (plots[i].title == NULL) {
                    continue;
                }

                int ybottom = 600 * (i + 1);
                int ytop    = ybottom - 600;
                plot_daily(&plots[i], psh, peh, ybottom, ytop);
            }
        }

        // present the display
        sdl_display_present();

        // wait for an event with 50 ms timeout;
        // if no event available, then redraw display
        // xxx could wait longer
        sdl_get_event(50000, &event);
        if (event.event_id == -1) {
            continue;
        }

        // process events
        switch (event.event_id) {
// xxx add event for end of data
        case EVID_QUIT:
            end_program = true;
            break;      
        case EVID_MOTION:
            if (hourly) {
                plot_end_time -= event.u.motion.xrel * (3600. * 24 / sdl_win_width);
            } else {
                plot_end_time -= event.u.motion.xrel * (86400. * NUM_DAYS / sdl_win_width);
            }
            break;      
        }

        // if end_program flag is set then break out of runtime loop
        if (end_program) {
            break;
        }
    }

    // cleanup and end program
    sdl_quit(SUBSYS_VIDEO);
    util_unmap_file(data);
    printf("INFO %s: terminating\n", progname);
    return 0;
}

void plot_daily(plot_t *p, int psh, int peh, int ybottom, int ytop)
{
    plot_point_t    pts_avg[MAX_PTS], pts_min[MAX_PTS], pts_max[MAX_PTS];
    void           *plot_cx;
    int             num_pts;

    get_plot_day_pts(p->which, psh, peh, pts_avg, pts_min, pts_max, &num_pts);

    plot_cx =  sdl_plot_create(p->title,
                               0, sdl_win_width-1,    // xleft, xright
                               ybottom, ytop,       // ybottom, ytop
                               psh, psh+NUM_DAYS*24,          // xval_left, xval_right
                               p->yval_bottom, p->yval_top,      // yval_bottom, yval_top
                               p->yval_of_x_axis);               // yval_of_x_axis

    sdl_plot_bars(plot_cx, pts_avg, pts_min, pts_max, num_pts, 24);

    sdl_plot_free(plot_cx);
}

void get_plot_pts(int which, int psh, int peh, plot_point_t *pts, int *num_pts)
{
    int hour, idx, n=0;
    double value;

    for (hour = psh; hour <= peh; hour++) {
        idx = hour - data->hdr.start_hour;
        if (idx < 0 || idx > data->hdr.last_idx || idx >= MAX_SENSOR_VALUES) {
            continue;
        }

        value = data->values[idx].sensors[which];
        if (value == INVALID_SENSOR_VALUE) {
            continue;
        }

        pts[n].xval = hour + 0.5;
        pts[n].yval = value;
        n++;

        // xxx limit value to min/max
    }

    *num_pts = n;
}

void get_plot_day_pts(
            int which, int psh, int peh,
            plot_point_t *pts_avg, plot_point_t * pts_min, plot_point_t * pts_max,
            int *num_pts)
{
    int    day_start_hour, hour, idx, k, n=0;
    double value, sum, avg, min, max;

    
    time_t t;
    struct tm tm;
    static bool print=true;

    for (day_start_hour = psh; day_start_hour <= peh; day_start_hour += 24) {
        if (print) {
            t = day_start_hour * 3600;
            localtime_r(&t, &tm);
            printf("%02d/%02d/%02d %02d:%02d:%02d\n",
                tm.tm_mon+1, tm.tm_mday, tm.tm_year-100,
                tm.tm_hour, tm.tm_min, tm.tm_sec);
        }

        sum = k = 0;
        min = 1e99;;
        max = -1e99;;
        for (hour = day_start_hour; hour < day_start_hour+24; hour++) {
            idx = hour - data->hdr.start_hour;
            if (idx < 0 || idx > data->hdr.last_idx || idx >= MAX_SENSOR_VALUES) {
                //if (print) printf("ERR idx out of range\n");
                continue;
            }
            if (print) {
                printf("  hour=%d idx=%d\n", hour, idx);
            }

            value = data->values[idx].sensors[which];
            if (value == INVALID_SENSOR_VALUE) {
                if (print) printf("ERR value invalid\n");
                continue;
            }

            sum += value;
            k++;

            if (value < min) min = value;
            if (value > max) max = value;
        }

        if (k == 0) {
            if (print) printf("ERR k=0\n");
            continue;
        }

        avg = sum / k;
        if (print) {
            printf("  k=%d  avg=%.1f  min=%.1f  max=%.1f\n", k, avg, min, max);
        }

        pts_avg[n].xval = day_start_hour + 12;
        pts_avg[n].yval = avg;

        pts_min[n].xval = day_start_hour + 12;
        pts_min[n].yval = min;

        pts_max[n].xval = day_start_hour + 12;
        pts_max[n].yval = max;
        n++;
    }

    *num_pts = n;
    if (print) {
        printf("  return %d pts\n", n);
    }

    print = false;
}
            

#if 0

// xxx is this used?
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
#endif
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
    double xval_span;
    double yval_min;
    double yval_max;
    double yval_span;
    double yval_of_x_axis;
} plot_cx_t;  // private


int xval2x(plot_cx_t *cx, double xval)
{
    int x;

    x = cx->xleft + (xval - cx->xval_min) * (cx->xspan / cx->xval_span);  // xxx add cvt constant to cx
    return x;
}

int yval2y(plot_cx_t *cx, double yval)
{
    int y;

    y = cx->ybottom - (yval - cx->yval_min) * (cx->yspan / cx->yval_span);  // xxx add cvt constant to cx
    return y;
}

void *sdl_plot_create(char *title, 
                      int xleft, int xright, int ybottom, int ytop,
                      double xval_left, int xval_right, double yval_bottom, int yval_top,
                      double yval_of_x_axis)
{
    plot_cx_t *cx;
    int i;

    // alloc cx and save params in cx
    cx = calloc(1, sizeof(plot_cx_t));
    strcpy(cx->title, title);
    cx->xleft          = xleft;
    cx->xright         = xright;
    cx->xspan          = xright - xleft;
    cx->ybottom        = ybottom;
    cx->ytop           = ytop;
    cx->yspan          = ybottom - ytop;
    cx->xval_min       = xval_left;
    cx->xval_max       = xval_right;
    cx->xval_span      = xval_right - xval_left;
    cx->yval_min       = yval_bottom;
    cx->yval_max       = yval_top;
    cx->yval_span      = yval_top - yval_bottom;
    cx->yval_of_x_axis = yval_of_x_axis;

    // draw y-axis on both left and right
    for (i = 0; i < 3; i++) {
        sdl_render_line(cx->xleft+i, cx->ybottom, cx->xleft+i, cx->ytop, COLOR_BLUE);
        sdl_render_line(cx->xright-i, cx->ybottom, cx->xright-i, cx->ytop, COLOR_BLUE);
    }

    // draw x-axis
    int y = yval2y(cx, yval_of_x_axis);
    for (i = -1; i <= 1; i++) {
        sdl_render_line(cx->xleft, y+i, cx->xright, y+i, COLOR_BLUE);
    }

    // label x and y axis
    // xxx

    // return cx
    return cx;
}

void sdl_plot_points(void *cx_arg, plot_point_t *pts, int num_pts)
{
    plot_cx_t   *cx = (plot_cx_t*)cx_arg;
    sdl_point_t *points;
    int          i, n=0, point_size=5;

    points = malloc(num_pts * sizeof(sdl_point_t));

    for (i = 0; i < num_pts; i++) {
        points[n].x = xval2x(cx, pts[i].xval);
        points[n].y = yval2y(cx, pts[i].yval);
        n++;
    }
    sdl_render_points(points, n, COLOR_WHITE, point_size);

    free(points);
}

void sdl_plot_bars(void *cx_arg, 
                   plot_point_t *pts_avg, plot_point_t *pts_min, plot_point_t *pts_max,
                   int num_pts, double bar_wval)
{
    int        i, x, y, w, h;
    double     wval, hval, xval, yval;
    plot_cx_t *cx = (plot_cx_t*)cx_arg;

    static bool first = 1;

    pts_min[0].yval = pts_max[0].yval = pts_avg[0].yval = 1010;

    for (i = 0; i < num_pts; i++) {
        wval = bar_wval;
        hval = pts_max[i].yval - pts_min[i].yval;
        xval = pts_min[i].xval - wval/2;
        yval = pts_max[i].yval;

        x = xval2x(cx, xval);
        y = yval2y(cx, yval);
        w = wval * cx->xspan / cx->xval_span;
        h = hval * cx->yspan / cx->yval_span;

        if (h < 7) {
            y -= (7-h) / 2;
            h = 7;
        }

        if (first) printf("%d: %f %f %f\n", i, 
                         pts_min[i].yval, pts_avg[i].yval, pts_max[i].yval);
        if (first) printf("    %d %d %d %d - hval=%f yspan=%d yval_span=%f\n", 
               x, y, w, h,
               hval, cx->yspan, cx->yval_span);

        sdl_render_fill_rect(x, y, w, h, COLOR_PURPLE);
    }

    sdl_plot_points(cx, pts_avg, num_pts);

    first = 0;
}

void sdl_plot_free(void *cx)
{
    // free cx
    free(cx);
}

