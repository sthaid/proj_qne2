#ifndef __SENSORS_H__
#define __SENSORS_H__

#define SENSOR_DATA_INITIALIZED 0x55aa66bb

// file size is approx 3.5 MB for 10 years of hourly data
#define MAX_SENSOR_VALUES (10 * 365 * 24)

typedef struct {
    struct {
        int initialized;
        int max;
        int next;
        int pad;
    } hdr;
    struct sensor_value_s {
        unsigned char month;
        unsigned char day;
        unsigned char year;
        unsigned char hour;
        unsigned char pad[4];
        double step_count;
        double pressure;
        double temperature;
        double humidity;
    } values[MAX_SENSOR_VALUES];
} sensors_data_t;

#endif
