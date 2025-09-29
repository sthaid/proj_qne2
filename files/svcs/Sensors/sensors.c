#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include <sdl.h>
#include <utils.h>

#include "svcs/Sensors/sensors.h"

// xxx todo 
// - check for file full (next == max)
// - add unit test code to simulate sensor data

// defines
#define SECS_PER_DAY 86400
#define SECS_PER_HOUR 3600
#define INTVL_SECS 10

#define TEST 1

// args
static char *progname;
static char *data_dir;
static int   id;

// prototypes
char *sensval2str(double x);
void add_simulated_values(sensors_data_t *data);

// xxxxxxxxxxxxxxxxxxxxxxxxxxxx
void twg(void)
{
    int filelen;
    char *buff;
    void *json;

    buff = util_read_file("svcs/Sensors", "xxyy.json", &filelen);
    // xxx error paths need to free buff
    if (buff == NULL) {
        printf("ERROR: failed to read json file\n");
        return;
    }
    printf("filelen = %d\n", filelen);

    json = util_json_parse(buff);
    if (json == NULL) {
        printf("ERROR: util_json_parse failed\n");
        return;
    }

    double number = util_json_get_number(json, "properties", "periods", "0", "temperature", NULL);
    if (number == NOT_A_NUMBER) {
        printf("ERROR: util_json_get_number failed\n");
        return;
    }
    printf("temperature = %f\n", number);

    // get more stuff
    util_json_free(json);
}

int main(int argc, char **argv)
{
    sensors_data_t        *data = NULL;
    int                    rc, idx, hour_last, hour_now;
    time_t                 t_now;
    double                 stepc_now, stepc_change, stepc_last;
    struct sensor_value_s *sv;

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

    // xxx test weather.gov
    twg();

    // init the SDL sensor subsystem
    rc = sdl_init(SUBSYS_SENSOR);
    if (rc != 0) {
        printf("ERROR %s: failed to init SUBSYS_SENSOR\n", progname);
        return 1;
    }

    // xxx
    if (TEST) {
        unlink("svcs_data/Sensors/sensors.dat");
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

        if (TEST) {
            add_simulated_values(data); // xxx comment out
        }

        util_sync_file(data, sizeof(sensors_data_t));

        printf("INFO %s: initialized sesnsors.dat, start_hour = %ld\n", progname, data->hdr.start_hour);
    } else {
        printf("INFO %s: sensors.dat mapped and version verified\n", progname);
    }

    // init variables used in the loop below
    sdl_sensor_read_step_counter(&stepc_last);
    hour_last = 0;

    // loop
    while (true) {
        // if request to stop received then break out of loop
        if (stop_requested[id]) {
            printf("INFO %s: got stop request\n", progname);
            break;
        }  

        // get hour now xxx comments througouth
        t_now = time(NULL);
        hour_now = t_now / SECS_PER_HOUR;
        idx = hour_now - data->hdr.start_hour;
        sv = &data->values[idx];

        // if hour has changed then add new sensor data to sensors.dat
        if (hour_now != hour_last) {
            // xxx comment
            //printf("INFO %s: hour has changed from %d -> %d\n", progname, hour_last, hour_now);
            if (idx < 0 || idx >= MAX_SENSOR_VALUES) {
                printf("ERROR %s: idx %d out of range 0 .. %d\n", progname, idx, MAX_SENSOR_VALUES);
                sleep(INTVL_SECS);
                continue;
            }

            // xxx
            if (TEST) {
                sv->sensors[STEP_COUNT]  = 0;
                sv->sensors[PRESSURE]    = 1000;
                sv->sensors[TEMPERATURE] = 20;
                sv->sensors[HUMIDITY]    = 50;
            } else {
                sv->sensors[STEP_COUNT] = 0;
                sdl_sensor_read_pressure(&sv->sensors[PRESSURE]);
                sdl_sensor_read_temperature(&sv->sensors[TEMPERATURE]);
                sdl_sensor_read_humidity(&sv->sensors[HUMIDITY]);
            }

            // update hdr.last_idx 
            data->hdr.last_idx = idx;
            util_sync_file(&data->hdr.last_idx, sizeof(data->hdr.last_idx));

            // debug print new sensor values
            struct tm tm;
            localtime_r(&t_now, &tm);
            printf("INFO %s: added values[%d] utc=%02d/%02d/%02d %02d:%02d:%02d) steps=%s press=%s temp=%s humid=%s\n",
                   progname, idx,
                   tm.tm_mon + 1, tm.tm_mday, tm.tm_year - 100, tm.tm_hour, tm.tm_min, tm.tm_sec,
                   sensval2str(sv->sensors[STEP_COUNT]),
                   sensval2str(sv->sensors[PRESSURE]), 
                   sensval2str(sv->sensors[TEMPERATURE]), 
                   sensval2str(sv->sensors[HUMIDITY]));

            // save hour_last for next iteration
            hour_last = hour_now;
        }

        // the step counter sensor value requires special attention
        // because the sensor value continuously increases; but what
        // is desired is the number of steps in the past interval
        sdl_sensor_read_step_counter(&stepc_now);
        if (stepc_now == INVALID_SENSOR_VALUE || stepc_last == INVALID_SENSOR_VALUE) {
            stepc_change = INVALID_SENSOR_VALUE;
        } else {
            stepc_change = stepc_now - stepc_last;
        }
        stepc_last = stepc_now;
        if (TEST) {
            stepc_change = 10;  //xxx
        }
        sv->sensors[STEP_COUNT] += stepc_change;
        printf("INFO %s: stepc_change=%.0f step_count=%.0f\n", progname, stepc_change, sv->sensors[STEP_COUNT]);

        // xxx
        util_sync_file(sv, sizeof(struct sensor_value_s));

        // sleep
        sleep(INTVL_SECS);
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

void add_simulated_values(sensors_data_t *data)
{
    int num_sim_hours = 1000;  // about 6 weeks
    int idx, i, hour_now;
    struct sensor_value_s *sv;

    printf("INFO %s: add_simulated_values starting\n", progname);

    // back off data start hour by 6 weeks
    data->hdr.start_hour -= num_sim_hours;

    // loop, filling in simulated sensor values 
    for (i = 0; i < num_sim_hours; i++) {
        hour_now = data->hdr.start_hour + i;
        idx = hour_now - data->hdr.start_hour;
        sv = &data->values[idx];

        sv->sensors[PRESSURE]    = 1000 + 50 * sin(i * (2 * M_PI / 240));
        sv->sensors[TEMPERATURE] = 70 + 30 * sin(i * (2 * M_PI / 240));
        sv->sensors[HUMIDITY]    = 60 + 20 * sin(i * (2 * M_PI / 240));
        sv->sensors[STEP_COUNT]  = 1000;

        data->hdr.last_idx = idx;
    }

    printf("INFO %s: add_simulated_values return, last_idx = %ld\n", progname, data->hdr.last_idx);
}
