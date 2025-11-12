#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <sdlx.h>
#include <utils.h>
#include <svcs.h>

#include "svcs/Location/location.h"

// variables
char *progname;
char *data_dir;

loc_hist_t *loc_hist;
    
// -----------------  MAIN  ------------------------------------------
    
int main(int argc, char **argv)
{
    int          rc;
    sdlx_event_t event;
    bool         done = false;
    int          created;

    // save args
    if (argc != 2) {
        printf("ERROR: data_dir arg expected\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    // init sdl video subsystem
    rc = sdlx_init(SUBSYS_VIDEO);
    if (rc != 0) {
        printf("ERROR %s: sdlx_init failed\n", progname);
        return 1;
    }

    // map location history file
    // xxx allow created to be NULL
    // - create_if_needed = false
    // - read_only = true 
    loc_hist = util_map_file("svcs/Location", LOC_HIST_FILENAME, sizeof(loc_hist_t), false, true, &created);
    if (loc_hist == NULL) {
        printf("ERROR: %s failed to map %s\n", progname, LOC_HIST_FILENAME);
        return 1; 
    }

    // xxx temp
    svc_req_t *req;

    req = calloc(1, sizeof(svc_req_t));
    req->req = SVC_LOCATION_REQ_GET_LOC_INFO;
    svc_issue_req("Location", req);
    svc_wait_for_req_complete(req, 10);


    if (req->comp_status != SVC_REQ_COMP_STATUS_OK) {
        printf("ERROR %s: req comp_status = %d\n", progname, req->comp_status);
    } else {
        req_get_loc_info_t x;
        memcpy(&x, req->data, sizeof(x));
        time_t t = x.out.t;
        struct tm tm;
        localtime_r(&t, &tm);
        printf("INFO %s: t=%02d:%02d lat/long=%0.3f,%0.3f miles=%0.3f name=%s\n",
              progname,
              tm.tm_hour, tm.tm_min, 
              x.out.latitude,
              x.out.longitude,
              x.out.miles,
              x.out.name);
    }

    // runtime loop
    while (!done) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // register control event to
        // - end program
        sdlx_register_control_events(NULL, NULL, "X", COLOR_BLACK, 0, 0, EVID_QUIT);

        // xxx display location info
        sdlx_render_printf_xyctr(sdlx_win_width/2, sdlx_win_height/2, "xxx Location");

        // present the display
        sdlx_display_present();

        // wait for event, with infinite timeout
        sdlx_get_event(-1, &event);

        // process events
        switch (event.event_id) {
        case EVID_QUIT:
            done = true;
            break;
        }
    }

    // cleanup and end program
    sdlx_quit(SUBSYS_VIDEO);
    printf("INFO %s: terminating\n", progname);
    return 0;
}
