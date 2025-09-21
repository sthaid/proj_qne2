#include <stdio.h>
#include <stdbool.h>
#include <time.h>
//#include <unistd.h>
//#include <string.h>
//#include <stdlib.h>

#include <sdl.h>
#include <utils.h>

#include "svcs/Sensors/sensors.h"

// xxx
// - use ints for sensors, and leave spares
// - buy wifi temperature sensor
// - check into auto start   SampleForegroundService
// - store time_t in sensors.dat, in addition to what is there

//
// defines
//

#define SECS_PER_HOUR 3600 //xxx

#define VERSION 1

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

// -----------------  MAIN  ------------------------------------------

#define MAX_STR_TBL 8
char *str(double x)
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

int main(int argc, char **argv)
{
    int             rc;
    sdl_event_t     event;
    bool            end_program = false;
    sensors_data_t *data;
    bool            first = true;

    // get args
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s version=%d\n", progname, data_dir, VERSION);

    // get params
    util_get_int_param(data_dir, "min_pressure", 950);
    util_get_int_param(data_dir, "max_pressure", 1050);

    // map the sensors.dat file
    data = util_map_file("svcs_data/Sensors", "sensors.dat", sizeof(sensors_data_t), false);
    if (data == NULL) {
        printf("ERROR %s: failed to map sensors.dat\n", progname);
        return 1;
    }

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

        // print last 10 sensor values
        int start_hour, last_idx, i, idx;
        double pressure, step_count;
        time_t t;
        struct tm tm;
        int r = 2;

        start_hour = data->hdr.start_hour;
        last_idx   = data->hdr.last_idx;
        for (i = 0; i < 10; i++) {
            idx = last_idx - i;
            if (idx < 0 || idx >= MAX_SENSOR_VALUES) {
                break;
            }

            t = (start_hour + idx) * SECS_PER_HOUR;
            pressure = data->values[idx].sensors[PRESSURE];
            step_count = data->values[idx].sensors[STEP_COUNT];
            gmtime_r(&t, &tm);

            if (first) {
                printf("INFO %s: utc=%02d/%02d/%02d %02d:%02d:%02d) pressure=%s  step_count=%s\n",
                   progname,
                   tm.tm_mon + 1, tm.tm_mday, tm.tm_year-100, tm.tm_hour, tm.tm_min, tm.tm_sec,
                   str(pressure), str(step_count));
            }

            sdl_render_printf(0, ROW2Y(r++), "%02d/%02d %02d %s %s",
                     tm.tm_mon+1, tm.tm_mday, tm.tm_hour, str(pressure), str(step_count));
        }
        first = false;

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
