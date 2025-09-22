#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include <sdl.h>
#include <utils.h>

#include "svcs/Sensors/sensors.h"

// xxx todo 
// - check for file full (next == max)
// - add unit test code to simulate sensor data

// defines
#define SECS_PER_DAY 86400
#define SECS_PER_HOUR 3600

// args
static char *progname;
static char *data_dir;
static int   id;

// prototypes
char *sensval2str(double x);

int main(int argc, char **argv)
{
    sensors_data_t *data = NULL;
    int             rc, idx, hour_last, hour_now;
    double          stepc_now, stepc_change, stepc_last;
    double          dummy;

    // save args
    if (argc != 3) {
        printf("ERROR: args expected: data_dir, id\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    sscanf(argv[2], "%d", &id);

    // print starting message
    printf("INFO %s: starting, data_dir = %s id = %d\n", progname, data_dir, id);
    printf("INFO %s: sensors.dat:\n", progname);
    printf("INFO %s:   version supported = %lx\n", progname, SENSORS_DATA_FILE_VERSION);
    printf("INFO %s:   size              = %zd\n", progname, sizeof(sensors_data_t));

    // init the SDL sensor subsystem
    rc = sdl_init(SUBSYS_SENSOR);
    if (rc != 0) {
        printf("ERROR %s: failed to init SUBSYS_SENSOR\n", progname);
        return 1;
    }

    // map the sensors.dat file, the file will be created if it doesnt exist 
    // or if the file exists but is the wrong size
    data = util_map_file(data_dir, "sensors.dat", sizeof(sensors_data_t), true);
    if (data == NULL) {
        printf("ERROR %s: failed to map or create sensors.dat\n", progname);
        return 1;
    }

    // if sensors.dat file is wrong version then
    // initialize the file's mapped data fields
    if (data->hdr.version != SENSORS_DATA_FILE_VERSION) {
        for (int i = 0; i < MAX_SENSOR_VALUES; i++) {
            for (int j = 0; j < MAX_SENSORS; j++) {
                data->values[i].sensors[j] = INVALID_SENSOR_VALUE;
            }
        }

        data->hdr.start_hour = time(NULL) / SECS_PER_HOUR;
        data->hdr.last_idx   = -1;
        data->hdr.pad        = 0;
        data->hdr.version = SENSORS_DATA_FILE_VERSION;

        util_sync_file(data, sizeof(sensors_data_t));

        printf("INFO %s: initialized sesnsors.dat, start_hour = %ld\n", progname, data->hdr.start_hour);
    } else {
        printf("INFO %s: sensors.dat mapped and version verified\n", progname);
    }

    // xxx comment
    sdl_sensor_read_step_counter(&dummy);
    sdl_sensor_read_pressure(&dummy);
    sdl_sensor_read_temperature(&dummy);
    sdl_sensor_read_humidity(&dummy);

    // init variables used in the loop below
    sdl_sensor_read_step_counter(&stepc_last);
    hour_last = time(NULL) / SECS_PER_HOUR;

    // loop
    while (true) {
        // if request to stop received then break out of loop
        if (stop_requested[id]) {
            printf("INFO %s: got stop request\n", progname);
            break;
        }  

        // get hour now
        hour_now = time(NULL) / SECS_PER_HOUR;

        // if hour has changed then add new sensor data to sensors.dat for 
        // the preceeding hour
        if (hour_now != hour_last) {
            // xxx comment
            //printf("INFO %s: hour has changed from %d -> %d\n", progname, hour_last, hour_now);
            idx = hour_now - data->hdr.start_hour - 1;
            if (idx < 0 || idx >= MAX_SENSOR_VALUES) {
                printf("ERROR %s: idx %d out of range\n", progname, idx);
                sleep(5);
                continue;
            }

            // the step counter sensor value requires special attention
            // because the sensor value continuously increases; but what
            // is desired is the number of steps in the past hour
            sdl_sensor_read_step_counter(&stepc_now);
            if (stepc_now == INVALID_SENSOR_VALUE || stepc_last == INVALID_SENSOR_VALUE) {
                stepc_change = INVALID_SENSOR_VALUE;
            } else {
                stepc_change = stepc_now - stepc_last;
            }
            stepc_last = stepc_now;

            // write sensor values to memory mapped sensors.dat, and
            // sync update to sensors.dat file
            struct sensor_value_s *x = &data->values[idx];
            x->sensors[STEP_COUNT] = stepc_change;
            sdl_sensor_read_pressure(&x->sensors[PRESSURE]);
            sdl_sensor_read_temperature(&x->sensors[TEMPERATURE]);
            sdl_sensor_read_humidity(&x->sensors[HUMIDITY]);
            util_sync_file(x, sizeof(struct sensor_value_s));

            // update hdr.last_idx 
            data->hdr.last_idx = idx;
            util_sync_file(&data->hdr.last_idx, sizeof(data->hdr.last_idx));

            // print values just added to sensors.dat
            time_t t = (data->hdr.start_hour + idx) * SECS_PER_HOUR;
            struct tm tm;
            gmtime_r(&t, &tm);
            printf("INFO %s: added values[%d] utc=%02d/%02d/%02d %02d:%02d:%02d) steps=%s pressure=%s temp=%s humidity=%s\n",
                   progname, idx,
                   tm.tm_mon + 1, tm.tm_mday, tm.tm_year - 100, tm.tm_hour, tm.tm_min, tm.tm_sec,
                   sensval2str(x->sensors[STEP_COUNT]),
                   sensval2str(x->sensors[PRESSURE]), 
                   sensval2str(x->sensors[TEMPERATURE]), 
                   sensval2str(x->sensors[HUMIDITY]));

            // save hour_last for next loop
            hour_last = hour_now;
        }

        // sleep 5 sec
        sleep(5);
    }

    // cleanup and end program
    util_unmap_file(data);
    sdl_quit(SUBSYS_SENSOR);
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
