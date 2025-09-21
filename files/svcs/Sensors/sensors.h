#ifndef __SENSORS_H__
#define __SENSORS_H__

#define SENSOR_DATA_INITIALIZED 0x55aa0003

#define MAX_SENSOR_VALUES (5 * 365 * 24)
#define MAX_SENSORS 8

#define STEP_COUNT   0
#define PRESSURE     1
#define TEMPERATURE  2
#define HUMIDITY     3

typedef struct {
    struct {
        unsigned long initialized;
        unsigned long start_hour;
        unsigned long last_idx;
        unsigned long pad;
    } hdr;
    struct sensor_value_s {
        //unsigned long time;
        //unsigned char month;
        //unsigned char day;
        //unsigned char year;
        //unsigned char hour;
        //unsigned char pad[4];
        double        sensors[MAX_SENSORS];
    } values[MAX_SENSOR_VALUES];
} sensors_data_t;

#endif
