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

// args
static char *progname;
static char *data_dir;
static int   id;

int main(int argc, char **argv)
{
    sensors_data_t *data = NULL;
    int             rc;
    time_t          t;
    struct tm       tm, tm_last;
    double          stepc_now, stepc_change, stepc_last;

    // save args
    if (argc != 3) {
        printf("ERROR: args expected: data_dir, id\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    sscanf(argv[2], "%d", &id);

    // print starting message
    printf("INFO %s: starting, data_dir = %s id = %d \n", progname, data_dir, id);
    printf("INFO %s: sizeof sensors_data_t = %zd\n", progname, sizeof(sensors_data_t));

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
        printf("ERROR %s: failed to map sensors.dat\n", progname);
        return 1;
    }

    // if sensors.dat is not initialized (it was probably just created) then
    // initialize non zero value fields
    if (data->hdr.initialized != SENSOR_DATA_INITIALIZED) {
        data->hdr.initialized = SENSOR_DATA_INITIALIZED;
        data->hdr.max = MAX_SENSOR_VALUES;
        util_sync_file(&data->hdr, sizeof(data->hdr));
    }

    // init variables used in the loop below
    sdl_sensor_read_step_counter(&stepc_last);
    t = time(NULL);
    localtime_r(&t, &tm_last);

    // loop
    while (true) {
        // if request to stop received then break out of loop
        if (stop_requested[id]) {
            printf("INFO %s: got stop request\n", progname);
            break;
        }  

        // get time now
        t = time(NULL);
        localtime_r(&t, &tm);

        // if hour has changed then add new sensor data to sensors.dat
        if (tm.tm_hour != tm_last.tm_hour) {
            struct sensor_value_s *x = &data->values[data->hdr.next];

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

            // init new sensor_value_s; 
            // note x->year is the actual year - 2000
            x->time  = t;
            x->month = tm_last.tm_mon + 1;
            x->day   = tm_last.tm_mday;
            x->year  = tm_last.tm_year - 100;
            x->hour  = tm_last.tm_hour;
            x->sensors[STEP_COUNT] = stepc_change;
            sdl_sensor_read_pressure(&x->sensors[PRESSURE]);
            sdl_sensor_read_temperature(&x->sensors[TEMPERATURE]);
            sdl_sensor_read_humidity(&x->sensors[HUMIDITY]);

            printf("INFO %s: %02d/%02d/%02d %02d: steps=%.0f pressure=%.0f temp=%.0f humidity=%.0f\n",
                   progname,
                   x->month, x->day, x->year, x->hour,
                   x->sensors[STEP_COUNT], 
                   x->sensors[PRESSURE], 
                   x->sensors[TEMPERATURE], 
                   x->sensors[HUMIDITY]);

            // sync value struct to file
            util_sync_file(x, sizeof(struct sensor_value_s));

            // increment hdr.next field, which will make the just
            // published sensor_value_s available to readers of the
            // sensors.dat file
            data->hdr.next++;
            util_sync_file(&data->hdr, sizeof(data->hdr));
        }

        // save tm for comparison on next loop
        tm_last = tm;

        // sleep 5 sec
        sleep(5);
    }

    // cleanup and end program
    util_unmap_file(data);
    sdl_quit(SUBSYS_SENSOR);
    printf("INFO %s: terminating\n", progname);
    return 0;
}

