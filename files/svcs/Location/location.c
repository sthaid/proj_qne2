#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <sdlx.h>
#include <utils.h>
#include <svcs.h>

#include "svcs/Location/loc_data.h"
#include "svcs/Location/location.h"

// defines
#define BOLTON_MASS_LATITUDE     42.4334
#define BOLTON_MASS_LONGITUDE   -71.6078

// typedefs
typedef struct {
    double latitude;
    double longitude;
    char   name[MAX_NAME];
} loc_t;

// variables
loc_hist_t *loc_hist;

// prototypes
void add_entry_to_loc_hist(time_t t, double latitude, double longitude, char *name, double miles);
char *most_recent_loc_hist_name(void);
void process_req(svc_req_t *req);

// xxx todo
// - improve picoc to use 64bit time_t
//   signed int time_t overflows in year 2038
// - update svc_wait_for_req to take 64 bit time
// - check sizeof native android time_t
// - support NAN in picoc
// - support popen either in utils or cstdlib

// -----------------  MAIN  -----------------------------------------

int main(int argc, char **argv)
{
    char          name[MAX_NAME];
    double        latitude, longitude, miles;
    unsigned long hour;
    svc_req_t    *req;
    int           rc;

    // save args
    if (argc != 2) {
        printf("ERROR: data_dir arg expected\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    // read location data
    rc = init_loc_data();
    if (rc != 0) {
        // xxx ERROR
    }

    // map the loc_hist file
    // xxx if it was created then init the magic
    loc_hist = util_map_file(data_dir, LOC_HIST_FILENAME, sizeof(loc_hist_t), true, false);
    if (loc_hist == NULL) {
        // xxx ERROR
    }

    // xxx
    hour = time(NULL) / 3600; //xxx + 1;

    // service runtime loop
    while (true) {
        // wait for req, or for current time to exceed hour
        svc_wait_for_req(progname, &req, hour*3600);

        // if req recvd then process the req
        if (req != NULL) {
            if (req->req == SVC_REQ_STOP) {
                svc_req_completed(req, SVC_REQ_COMP_STATUS_OK);
                break;  // xxx process this in process_req
            }
            process_req(req);
        }

        // if current time is greater than hour then
        // - find location in database that is closest to current lat/long;
        // - if name is different than most recent entry in loc_file
        //    then add new entry to loc file, 
        // - increment hour
        // endif
        if (time(NULL) > hour * 3600) {
            // find location in database that is closest to current lat/long;
            util_get_location(&latitude, &longitude, NULL);  // xxx check for no lat/long
            find_closest_loc_data(latitude, longitude, name, &miles);  // xxx dont overflow name 

            // if name is different than most recent entry in loc_file
            // then add new entry to loc file, 
            if (strcmp(most_recent_loc_hist_name(), name) != 0) {
                add_entry_to_loc_hist(time(NULL), latitude, longitude, name, miles);
            }

            // increment hour
            hour++;
        }
    }

    // cleanup and end program
    free(loc_data);
    util_unmap_file(loc_hist, sizeof(loc_hist_t));
    printf("INFO %s: terminating\n", progname);
    return 0;
}

// -----------------  LOC_HIST FILE: UPDATE & QUERY  ----------------

void add_entry_to_loc_hist(time_t t, double latitude, double longitude, char *name, double miles)
{
    struct loc_hist_entry_s *x;

    x = &loc_hist->loc[loc_hist->count % MAX_LOC_HIST];

    x->t          = t;
    x->latitude   = latitude;
    x->longitude  = longitude;
    strcpy(x->name, name);
    x->miles      = miles;

    loc_hist->count++;

    // xxx sync
}

char *most_recent_loc_hist_name(void)
{
    if (loc_hist->count == 0) {
        return "";
    } else {
        return loc_hist->loc[(loc_hist->count-1) % MAX_LOC_HIST].name;
    }    
}

// -----------------  PROCESS REQ FROM CLIENT APP  ------------------

void process_req(svc_req_t *req)
{
    switch (req->req) {
    case SVC_LOCATION_REQ_GET_LOC_INFO:
        //find_closest_loc_data(name, &miles);
        svc_req_completed(req, SVC_REQ_COMP_STATUS_OK);
        break;
    case SVC_LOCATION_REQ_ADD_COUNTRY_INFO:
        //download_country_info("us");
        svc_req_completed(req, SVC_REQ_COMP_STATUS_OK);
        break;
    case SVC_LOCATION_REQ_DEL_COUNTRY_INFO:
        svc_req_completed(req, SVC_REQ_COMP_STATUS_OK);
        break;
    case SVC_LOCATION_REQ_LIST_COUNTRY_INFO:
        svc_req_completed(req, SVC_REQ_COMP_STATUS_OK);
        break;
    default:
        svc_req_completed(req, SVC_REQ_COMP_STATUS_ERROR_INVALID_REQ);
        // xxx ERROR
    }
}

