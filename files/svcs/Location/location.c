#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

#include <sdlx.h>
#include <utils.h>
#include <svcs.h>

#include "svcs/Location/loc_data.h"
#include "svcs/Location/location.h"

// typedefs
typedef struct {
    double latitude;
    double longitude;
    char   name[MAX_NAME];
} loc_t;

// variables
loc_hist_t *loc_hist;
bool end_program = false;

// prototypes
void add_entry_to_loc_hist(time_t t, double latitude, double longitude, char *name, double miles);
char *most_recent_loc_hist_name(void);
void process_req(svc_req_t *req);

// xxx todo
// - improve picoc to use 64bit time_t
//   signed int time_t overflows in year 2038
// - check sizeof native android time_t
// - support NAN in picoc

// -----------------  MAIN  -----------------------------------------

int main(int argc, char **argv)
{
    char          name[MAX_NAME];
    double        latitude, longitude, miles;
    long          abstime;
    svc_req_t    *req;
    int           rc, created;

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
        printf("ERROR: %s failed to read location data\n", progname);
        return 1;
    }

    // map the loc_hist file
    loc_hist = util_map_file(data_dir, LOC_HIST_FILENAME, sizeof(loc_hist_t), true, false, &created);
    if (loc_hist == NULL) {
        printf("ERROR: %s failed to map %s\n", progname, LOC_HIST_FILENAME);
        return 1;
    }

    // set absolute time at which svc_wait_for_req will timeout;
    // this time is rounded down to the prior hour so that the first
    // call to svc_wait_for_req will timeout immedeately
    abstime = time(NULL) / 3600 * 3600;

    // service runtime loop
    while (!end_program) {
        // wait for req or timeout
        rc = svc_wait_for_req(progname, &req, abstime);

        // if svc_wait_for_req timedout
        // - find location in database that is closest to current lat/long;
        // - if name is different than most recent entry in loc_file
        //    then add new entry to loc file, 
        // - increment abstime
        // endif
        if (rc == SVC_WAIT_FOR_REQ_ERROR_TIMEDOUT) {
            // find location in database that is closest to current lat/long;
            util_get_location(&latitude, &longitude, NULL);  // xxx check for no lat/long
            find_closest_loc_data(latitude, longitude, name, &miles);  // xxx dont overflow name 

            // if name is different than most recent entry in loc_file
            // then add new entry to loc file, 
            if (strcmp(most_recent_loc_hist_name(), name) != 0) {
                add_entry_to_loc_hist(time(NULL), latitude, longitude, name, miles);
            }

            // increment abstime
            abstime += 3600;
            continue;
        }

        // if svc_wait_for_req had some error other than the timeout handled above,
        // then short sleep and contune; perhaps the error will clear up
        if (rc != SVC_WAIT_FOR_REQ_SUCCESS) {
            sleep(10);
            continue;
        }

        // if req was recvd then process the req
        if (req != NULL) {
            process_req(req);
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

    util_sync_file(loc_hist, sizeof(loc_hist_t));
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
    case SVC_REQ_STOP:
        svc_req_completed(req, SVC_REQ_COMP_STATUS_OK);
        end_program = true;
        break;
    case SVC_LOCATION_REQ_GET_LOC_INFO: {
        double              latitude, longitude, miles;
        char                name[MAX_NAME];
        req_get_loc_info_t *x = (req_get_loc_info_t*)req->data;

        util_get_location(&latitude, &longitude, NULL);
        find_closest_loc_data(latitude, longitude, name, &miles);

        x->out.t          = time(NULL);
        x->out.latitude   = latitude;
        x->out.longitude  = longitude;
        x->out.miles      = miles;
        strcpy(x->out.name, name);

        svc_req_completed(req, SVC_REQ_COMP_STATUS_OK);
        break; }
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
        printf("ERROR %s: req %d is invalid\n", progname, req->req);
        svc_req_completed(req, SVC_REQ_COMP_STATUS_ERROR_INVALID_REQ);
        break;
    }
}

