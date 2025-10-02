#ifndef __SENSORS_H__
#define __SENSORS_H__

#define SENSORS_DATA_FILE_VERSION 0x55aa55aa55aa0004

#define MAX_SENSOR_VALUES (5 * 365 * 24)
#define MAX_SENSORS 8

#define STEP_COUNT   0
#define PRESSURE     1
#define TEMPERATURE  2
#define HUMIDITY     3

typedef struct {
    struct {
        unsigned long version;
        unsigned long start_hour;
        unsigned long last_idx;
        unsigned long pad;
    } hdr;
    struct sensor_value_s {
        double sensors[MAX_SENSORS];
    } values[MAX_SENSOR_VALUES];
} sensors_data_t;

#endif
