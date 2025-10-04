#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#include <sdl.h>
#include <utils.h>

#include <svcs/Sensors/common.h>
#include <svcs/Sensors/sensors.h>

// defines
#define VERSION       1.0
#define SECS_PER_HOUR 3600
#define INTVL_SECS    10

// prototypes
static char *sensval2str(double x);

// -----------------  MAIN  -------------------------------------

int main(int argc, char **argv)
{
    sensors_data_t        *data = NULL;
    int                    rc, idx, hour_last, hour_now;
    time_t                 t_now;
    double                 stepc_now, stepc_last;
    struct sensor_value_s *sv;
    double                 weather_gov_temperature, weather_gov_relhumidity;
    int                    id;

    // save args
    if (argc != 3) {
        printf("ERROR: args expected: data_dir, id\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    sscanf(argv[2], "%d", &id);

    // print starting message
    printf("INFO %s: starting: version=%d data_dir=%s id=%d\n",
           progname, VERSION, data_dir, id); // xxx use same fmt in all apps
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
    // or if the file exists and is the wrong size
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
                data->values[i].sensors[j] = INVALID_NUMBER;
            }
        }

        data->hdr.start_hour = time(NULL) / SECS_PER_HOUR;
        data->hdr.last_idx   = -1;
        data->hdr.pad        = 0;
        data->hdr.version    = SENSORS_DATA_FILE_VERSION;

        util_sync_file(data, sizeof(sensors_data_t));

        printf("INFO %s: initialized sesnsors.dat, start_hour = %ld\n", progname, data->hdr.start_hour);
    } else {
        printf("INFO %s: sensors.dat mapped and version verified\n", progname);
    }

    // init variables used in the runtime loop below
    stepc_last = INVALID_NUMBER;
    hour_last = 0;

    // runtime loop
    while (true) {
        // if request to stop received then break out of loop
        if (stop_requested[id]) {
            printf("INFO %s: got stop request\n", progname);
            break;
        }  

        // init the following:
        // - hour_now: which is the number of hours since the Unix Epoch
        // - idx: index of sensor data in the sensors.dat file
        // - sv: pointer to sensor values for this idx
        t_now = time(NULL);
        hour_now = t_now / SECS_PER_HOUR;
        idx = hour_now - data->hdr.start_hour;
        sv = &data->values[idx];

        // if hour has changed then add new sensor data to sensors.dat
        if (hour_now != hour_last) {
            // if idx is out of range then print an error and continue;
            // xxx todo, shift the data by one year to recover for idx too big
            if (idx < 0 || idx >= MAX_SENSOR_VALUES) {
                printf("ERROR %s: idx %d out of range 0 .. %d\n", progname, idx, MAX_SENSOR_VALUES);
                sleep(INTVL_SECS);
                continue;
            }

            // init the stepcount to 0, this will be added to periodically 
            // during the hour by code that follows
            sv->sensors[ASENSOR_STEP_COUNT] = 0;

            // read android sensor values; these are read just this one time
            // at the begining of the hour
            sdl_sensor_read_pressure(&sv->sensors[ASENSOR_PRESSURE]);
            sdl_sensor_read_temperature(&sv->sensors[ASENSOR_TEMPERATURE]);
            sdl_sensor_read_humidity(&sv->sensors[ASENSOR_HUMIDITY]);

            // get values from weather.gov
            get_weather(&weather_gov_temperature, &weather_gov_relhumidity);
            sv->sensors[WEATHER_GOV_TEMPERATURE] = weather_gov_temperature;
            sv->sensors[WEATHER_GOV_RELHUMIDITY] = weather_gov_relhumidity;

            // set sensors.dat hdr last_idx, to the new idx
            data->hdr.last_idx = idx;
            util_sync_file(&data->hdr.last_idx, sizeof(data->hdr.last_idx));

            // debug print new sensor values just added at the new idx
            struct tm tm;
            localtime_r(&t_now, &tm);
            printf("INFO %s: adding values[%d] utc=%02d/%02d/%02d %02d:%02d:%02d\n",
                   progname, idx,
                   tm.tm_mon + 1, tm.tm_mday, tm.tm_year - 100, tm.tm_hour, tm.tm_min, tm.tm_sec);
            printf("INFO %s:   ASENSOR press=%s temp=%s humid=%s\n",
                   progname,
                   sensval2str(sv->sensors[ASENSOR_PRESSURE]), 
                   sensval2str(sv->sensors[ASENSOR_TEMPERATURE]), 
                   sensval2str(sv->sensors[ASENSOR_HUMIDITY]));
            printf("INFO %s:   WEATHER_GOV temp=%s humid=%s\n", 
                   progname,
                   sensval2str(sv->sensors[WEATHER_GOV_TEMPERATURE]),
                   sensval2str(sv->sensors[WEATHER_GOV_RELHUMIDITY]));

            // save hour_last for next iteration
            hour_last = hour_now;
        }

        // the step counter sensor value needs special attention
        // because the sensor value continuously increases; 
        // this code accumulates the number of steps taken during this hour
        sdl_sensor_read_step_counter(&stepc_now);
        if (stepc_now != INVALID_NUMBER && stepc_last != INVALID_NUMBER) {
            sv->sensors[ASENSOR_STEP_COUNT] += (stepc_now - stepc_last);
            if (stepc_now > stepc_last) {
                printf("INFO %s: step_count=%.0f\n", progname, sv->sensors[ASENSOR_STEP_COUNT]);
            }
        }
        stepc_last = stepc_now;

        // sync the sensor values from memory to the sensors.dat file
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

// -----------------  UTILS  --------------------------------------------

#define MAX_STR_TBL 15

static char *sensval2str(double x)
{
    static char str_tbl[MAX_STR_TBL][30];
    static int  n;
    char *s;

    if (x == INVALID_NUMBER) {
        return "invld";
    } else {
        s = str_tbl[n];
        n = (n + 1) % MAX_STR_TBL;
        sprintf(s, "%.0f", x);
        return s;
    }
}

