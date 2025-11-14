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

#include "svcs/Location/location.h"
#include "svcs/Location/common.h"

// variables
loc_hist_t *loc_hist;
bool        end_program = false;
bool        test_loc_hist = true;

// prototypes
void add_entry_to_loc_hist(time_t t, double latitude, double longitude, char *name, double miles);
char *most_recent_loc_hist_name(void);
void create_loc_data_str(time_t t, double latitude, double longitude, char *name, double miles, char *data_str);
void add_simulated_entries_to_loc_hist(void);
void process_req(svc_req_t *req);
double rand_double(void);

// -----------------  MAIN  -----------------------------------------

int main(int argc, char **argv)
{
    char          name[MAX_NAME];
    double        latitude, longitude, miles;
    long          abstime;
    svc_req_t    *req;
    int           rc;
    int           created;

    // save args
    if (argc != 2) {
        printf("ERROR: data_dir arg expected\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    // read location data
    rc = read_loc_data();
    if (rc != 0) {
        printf("ERROR: %s failed to read location data\n", progname);
        return 1;
    }

    // map the loc_hist file
    // - create_if_needed = true
    // - read_only = false
    // - created (return flag) = NULL
    loc_hist = util_map_file(data_dir, LOC_HIST_FILENAME, sizeof(loc_hist_t), true, false, &created);
    if (loc_hist == NULL) {
        printf("ERROR: %s failed to map %s\n", progname, LOC_HIST_FILENAME);
        return 1;
    }

    // when test_mode is enabled and the loc_hist file was just created,
    // add simulated entries to the loc_hist file
    if (test_loc_hist && created) {
        add_simulated_entries_to_loc_hist();
    }

    // set absolute time at which svc_wait_for_req will timeout;
    // this time is rounded down to the prior hour so that the first
    // call to svc_wait_for_req will timeout immedeately
    abstime = time(NULL) / 3600 * 3600;

    // service runtime loop
    while (!end_program) {
        // wait for req or timeout
        rc = svc_wait_for_req(progname, &req, abstime);

        // if an unexpected error is returned, then delay and try again
        if (rc != 0 && rc != SVC_WAIT_FOR_REQ_ERROR_TIMEDOUT) {
            printf("ERROR %s: svc_wait_for_req returned unexpected error %d\n", progname, rc);
            sleep(1);
            continue;
        }

        // if svc_wait_for_req timedout
        // - find location in database that is closest to current lat/long;
        // - if name is different than most recent entry in loc_file
        //    then add new entry to loc file, 
        // - increment abstime
        // endif
        if (rc == SVC_WAIT_FOR_REQ_ERROR_TIMEDOUT) {
            // find location in database that is closest to current lat/long;
            util_get_location(&latitude, &longitude, NULL);  // xxx check for no lat/long
            find_closest_loc_data(latitude, longitude, name, &miles);

            // if name is different than most recent entry in loc_file
            // then add new entry to loc file, 
            if (strcmp(most_recent_loc_hist_name(), name) != 0) {
                add_entry_to_loc_hist(time(NULL), latitude, longitude, name, miles);
            }

            // update abstime to next hour
            abstime += 3600;
            continue;
        }

        // if req was recvd then process the req
        if (req != NULL) {
            process_req(req);
        }
    }

    // cleanup and end program
    free_loc_data();
    util_unmap_file(loc_hist, sizeof(loc_hist_t));
    printf("INFO %s: terminating\n", progname);
    return 0;
}

// -----------------  LOC_HIST SUPPORT  -----------------------------




void add_entry_to_loc_hist(time_t t, double latitude, double longitude, char *name, double miles)
{
    create_loc_data_str(t, latitude, longitude, name, miles,
                        loc_hist->loc[loc_hist->count].data_str);

    loc_hist->count++;

    util_sync_file(loc_hist, sizeof(loc_hist_t));

    // xxx handle file full
}

char *most_recent_loc_hist_name(void)
{
    static char name[MAX_NAME];
    char *ptr, *data_str;

    if (loc_hist->count == 0) {
        return "";
    }

    data_str = loc_hist->loc[loc_hist->count-1].data_str;

    ptr = strchr(data_str, '\n');
    if (ptr == NULL) {
        printf("ERROR %s: newline char not found in data_str '%s'\n", progname, data_str);
        return "";
    }

    memcpy(name, data_str, ptr-data_str);
    name[ptr-data_str] = '\0';

    printf("INFO %s: most recent name = '%s'\n", progname, name);
    return name;
}

void create_loc_data_str(time_t t, double latitude, double longitude, char *name, double miles, char *data_str)
{
    struct tm *tm;
    char time_str[50];

    // Bolton
    // --------------------
    // 06/05/2025 23:00 EST
    // Jun 5 2025 23:00 EST
    // -42.1234 -130.1234

    // create time string
    tm = localtime(&t);
    strftime(time_str, sizeof(time_str), "%b %d %Y %H:%M %Z", tm);

    // sprint location info to str
    sprintf(data_str, "%s\n%s\n%0.4f %0.4f\n\n", name, time_str, latitude, longitude);
}

void add_simulated_entries_to_loc_hist(void)
{
    double latitude, longitude, miles;
    char name[MAX_NAME];
    time_t t;

    t = time(NULL) - 30 * 86400;
    t = t / 3600 * 3600;

    for (int i = 0; i < 20; i++) {
        // get random location in Massachusett
        latitude  = 41.23 + (42.88 - 41.23) * rand_double();
        longitude = -(69.93 + (73.50 - 69.93) * rand_double());

        // find closest location from loc_data
        find_closest_loc_data(latitude, longitude, name, &miles);

        // add to loc_hist file
        add_entry_to_loc_hist(t, latitude, longitude, name, miles);

        // advance time one hour
        t += 3600;
    }
}

// -----------------  PROCESS REQ SUPPORT  --------------------------

void process_req(svc_req_t *req)
{
    switch (req->req) {
    case SVC_REQ_STOP:
        svc_req_completed(req, SVC_REQ_STATUS_OK);
        end_program = true;
        break;
    case SVC_LOCATION_REQ_GET_LOC_INFO: {
        double latitude, longitude, miles;
        char name[MAX_NAME];

        util_get_location(&latitude, &longitude, NULL);
        find_closest_loc_data(latitude, longitude, name, &miles);
        create_loc_data_str(time(NULL), latitude, longitude, name, miles, req->data);

        svc_req_completed(req, SVC_REQ_STATUS_OK);
        break; }
    case SVC_LOCATION_REQ_ADD_COUNTRY_INFO:
        //download_country_info("us");
        svc_req_completed(req, SVC_REQ_STATUS_OK);
        break;
    case SVC_LOCATION_REQ_DEL_COUNTRY_INFO:
        svc_req_completed(req, SVC_REQ_STATUS_OK);
        break;
    case SVC_LOCATION_REQ_LIST_COUNTRY_INFO:
        svc_req_completed(req, SVC_REQ_STATUS_OK);
        break;
    default:
        printf("ERROR %s: req %d is invalid\n", progname, req->req);
        svc_req_completed(req, SVC_REQ_STATUS_ERROR_INVALID_REQ);
        break;
    }
}

// -----------------  MISC UTILS  -----------------------------------

double rand_double(void)
{
    double rand;

    rand = (double)random() / 0x7fffffff;
    printf("%f\n", rand);
    return rand;
}
