#include <stdio.h>
#include <stdbool.h>
//#include <unistd.h>
//#include <time.h>
//#include <string.h>
//#include <stdlib.h>

#include <sdl.h>
#include <utils.h>

#include "svcs/sensors/sensors.h"

//
// defines
//

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

    // save args
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    // init sdl
    rc = sdl_init(SUBSYS_VIDEO);
    if (rc != 0) {
        printf("ERROR %s: sdl_init failed\n", progname);
        return 1;
    }

    while (true) {
        // init the backbuffer, and print font/color
        sdl_display_init(COLOR_BLACK);
        sdl_print_init(DEFAULT_FONT, COLOR_WHITE, COLOR_BLACK);

        // register control events
        // 'X' - end prorgram
        sdl_register_control_events(NULL, NULL, "X", 
                                    COLOR_BLACK,
                                    0, 0, EVID_QUIT);

        // register xxx events
        sdl_register_event(NULL, EVID_MOTION);

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
