#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <sdlx.h>
#include <utils.h>
#include <svcs.h>

#include "svcs/Location/location.h"

// defines
#define EVID_SETTINGS  10
#define EVID_GOTO_TOP  11

#define SEC 1000000

#define Y_CURR_LOC 50  // xxx check in clock

// variables
char *progname;
char *data_dir;
    
// prototypes
// xxx move to svcs
char *svc_make_req(char *svc_name, int req_id, char *data_in, int timeout_secs);
void settings(void);

// NOTES
// Bolton
// 6/5/2025 23:00 EST
// -42.1234 -130.1234
//
// 123456789 123456789


// xxx
// - Name Not Found
// -----------------  MAIN  ------------------------------------------
    
int main(int argc, char **argv)
{
    int          rc;
    sdlx_event_t event;
    bool         done = false;
    loc_hist_t *loc_hist;

    double y_top;
    int y_display_begin;
    int y_display_end;

    time_t time_now;
    time_t time_last_get_loc_info = 0;

    char loc_curr[MAX_SVC_REQ_DATA] = "Not Initialized";
    char *loc_curr_lines[1] = {loc_curr};
    char *loc_hist_lines[MAX_LOC_HIST];

    bool settings_changed = false;

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

    // init
    y_top = ROW2Y(5); // xxx adjust
    y_display_begin = ROW2Y(5);
    y_display_end = sdlx_win_height-2*sdlx_char_height;  // need a define or routine for this ?

    // map location history file
    // - create_if_needed = false
    // - read_only = true 
    // - created (return flag) = NULL
    loc_hist = util_map_file("svcs/Location", LOC_HIST_FILENAME, sizeof(loc_hist_t), false, true, NULL);
    if (loc_hist == NULL) {
        printf("ERROR: %s failed to map %s\n", progname, LOC_HIST_FILENAME);
        return 1; 
    }

    // if location history file was created, and test mode is enabled then
    // add simulated entries to the loc hist file
    // xxx ^^^ this is for the service

    // runtime loop
    while (!done) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // get and display current location
        time_now = time(NULL);
        if (time_now - time_last_get_loc_info > 60 || settings_changed) {
            char *response = svc_make_req("Location", SVC_LOCATION_REQ_GET_LOC_INFO, NULL, 5);
            if (response == NULL) {
                strcpy(loc_curr, "Loc Svc Error");
            } else {
                strncpy(loc_curr, response, MAX_SVC_REQ_DATA);
                loc_curr[MAX_SVC_REQ_DATA-1] = '\0';
            }
            time_last_get_loc_info = time_now;
            settings_changed = false;
        }
        sdlx_render_multiline_text(Y_CURR_LOC, Y_CURR_LOC, Y_CURR_LOC+3*sdlx_char_height, loc_curr_lines, 1);

        // display the location history
        int count = loc_hist->count;
        for (int i = 0; i < count; i++) {
            loc_hist_lines[i] = loc_hist->loc[count-1-i].data_str;
        }
        sdlx_render_multiline_text(y_top, y_display_begin, y_display_end, loc_hist_lines, loc_hist->count);

        // register for events
        sdlx_register_event(NULL, EVID_MOTION);
        sdlx_register_control_events("stg", "top", "X", 
                                     COLOR_WHITE, COLOR_BLACK, 
                                     EVID_SETTINGS, EVID_GOTO_TOP, EVID_QUIT);

        // present the display
        sdlx_display_present();

        // wait for event, with 10 second timeout
        sdlx_get_event(10*SEC, &event);

        // process events
        switch (event.event_id) {
        case EVID_QUIT:
            done = true;
            break;
        case EVID_SETTINGS:
            settings();
            settings_changed = true;
            y_top = ROW2Y(5); // xxx adjust AND needs define
            break;
        case EVID_GOTO_TOP:
            y_top = ROW2Y(5); // xxx adjust AND needs define
            break;
        case EVID_MOTION:
            y_top += event.u.motion.yrel;
            if (y_top >= y_display_begin) {
                y_top = y_display_begin;
            }
            //y_top += xxx;
            break;
        }
    }

    // cleanup and end program
    sdlx_quit(SUBSYS_VIDEO);
    printf("INFO %s: terminating\n", progname);
    return 0;
}

// -----------------  SETTINGS  ----------------------------------------

#define EVID_DEL_COUNTRY 20  // through 24
#define EVID_ADD_COUNTRY 30

#define MAX_COUNTRIES 5

char countries[MAX_COUNTRIES][3];
int  max_countries;

void get_countries(void);

void settings(void)
{
    bool done = false;
    sdlx_loc_t *loc;
    sdlx_event_t event;

    while (!done) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // get list of countries
        get_countries();

        // display list of countries, with DEL event for each
        for (int i = 0; i < max_countries; i++) {
            sdlx_render_printf(0, ROW2Y(i+1), "%s", countries[i]);

            sdlx_print_init_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
            loc = sdlx_render_printf(COL2X(10), ROW2Y(i+1), "%s", "DEL");
            sdlx_register_event(loc, EVID_DEL_COUNTRY+i);
            sdlx_print_init_color(COLOR_WHITE, COLOR_BLACK);
        }

        // register for Download Country event
        sdlx_print_init_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
        loc = sdlx_render_printf(0, ROW2Y(10), "%s", "Download Country");
        sdlx_print_init_color(COLOR_WHITE, COLOR_BLACK);
        sdlx_register_event(loc, EVID_ADD_COUNTRY);

        // register for quit event
        sdlx_register_control_events(NULL, NULL, "X", 
                                     COLOR_WHITE, COLOR_BLACK, 
                                     0, 0, EVID_QUIT);

        // present the display
        sdlx_display_present();

        // wait for event, infinite timeout
        sdlx_get_event(-1, &event);

        // process events
        switch (event.event_id) {
        case EVID_QUIT:
            done = true;
            break;

        case EVID_ADD_COUNTRY: {
            char *response, *country_code;

            // xxx fg color too?
            // xxx also multi line prompt, with notice about time
            // xxx force ctry code to lowercase
            country_code = sdlx_get_input_str("2 Char Country Code?", false, COLOR_BLACK);
            if (country_code == NULL) {
                break;
            }
            printf("COUNTRY CODE '%s'\n", country_code);

            printf("INFO %s: downloading %s\n", progname, country_code);
            response = svc_make_req("Location",      
                                    SVC_LOCATION_REQ_ADD_COUNTRY_INFO,
                                    country_code,
                                    60);    // timeout_secs  xxx how long
            if (response == NULL) {
                printf("ERROR %s: SVC_LOCATION_REQ_ADD_COUNTRY_INFO '%s' failed\n", progname, country_code);
            }
            break; }

        case EVID_DEL_COUNTRY+0:
        case EVID_DEL_COUNTRY+1:
        case EVID_DEL_COUNTRY+2:
        case EVID_DEL_COUNTRY+3:
        case EVID_DEL_COUNTRY+4: {
            int idx = event.event_id - EVID_DEL_COUNTRY;
            char *response;

            printf("INFO %s: deleteing %s\n", progname, countries[idx]);
            response = svc_make_req("Location",      
                                    SVC_LOCATION_REQ_DEL_COUNTRY_INFO,
                                    countries[idx],
                                    5);    // timeout_secs
            if (response == NULL) {
                printf("ERROR %s: SVC_LOCATION_REQ_DEL_COUNTRY_INFO '%s' failed\n", progname, countries[idx]);
            }
            break; }
        }
    }
}

void get_countries(void)
{
    char *response, *p;

    memset(countries, 0, sizeof(countries));
    max_countries = 0;

    response = svc_make_req("Location",      
                            SVC_LOCATION_REQ_LIST_COUNTRY_INFO,
                            NULL,  // data_in = NULL
                            5);    // timeout_secs
    if (response == NULL) {
        printf("ERROR %s: SVC_LOCATION_REQ_LIST_COUNTRY_INFO failed\n", progname);
    }

    while (true) {
        p = strchr(response, '\n');
        if (p == NULL) {
            break;
        }

        *p = '\0';
        snprintf(countries[max_countries], sizeof(countries[max_countries]), "%s", response);
        max_countries++;
        response = p + 1;

        if (max_countries == MAX_COUNTRIES) {
            break;
        }
    }
}

// -----------------  UTILS     ----------------------------------------

// xxx move to svcs
char *svc_make_req(char *svc_name, int req_id, char *data_in, int timeout_secs)
{
    svc_req_t *req;
    static char data_out[MAX_SVC_REQ_DATA];

    req = calloc(1, sizeof(svc_req_t));

    req->req = req_id;
    if (data_in) {
        strncpy(req->data, data_in, MAX_SVC_REQ_DATA);
        req->data[MAX_SVC_REQ_DATA-1] = '\0';
    }
    svc_issue_req(svc_name, req);
    svc_wait_for_req_complete(req, timeout_secs);

    if (req->status != SVC_REQ_STATUS_OK) {
        printf("ERROR %s: svc_make_req failed, req->req=%d req->status=%d\n", 
               progname, req->req, req->status);
        return NULL;
    }

    strncpy(data_out, req->data, MAX_SVC_REQ_DATA);
    data_out[MAX_SVC_REQ_DATA-1] = '\0';
    printf("xxxxxxx data out '%s'\n", data_out);

    free(req);

    return data_out;
}

