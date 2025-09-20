#include <stdio.h>
#include <stdbool.h>
//#include <unistd.h>
//#include <time.h>
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
static bool  end_program;

//
// prototypes
//

// -----------------  MAIN  ------------------------------------------

int main(int argc, char **argv)
{
    int rc;
    sdl_event_t event;

    // get args
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    // get params
    util_get_int_param(data_dir, "min_pressure", 950);
    util_get_int_param(data_dir, "max_pressure", 1050);

    // map the sensors.dat file
    data = util_map_file("svcs_data/Sensors" "sensors.dat", sizeof(sensors_data_t), false);
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

        // register xxx events
        //sdl_register_event(NULL, EVID_MOTION);
        idx = data->hdr.next-1;
        plot(idx,
             params.min_pressure,
             params.max_pressure,
             DURATION_WEEK,
             PRESSURE_SENSOR);


idx, pressure, ymin, ymax, duration);
        

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

    // return success
    printf("INFO %s: terminating\n", progname);
    return 0;
}
