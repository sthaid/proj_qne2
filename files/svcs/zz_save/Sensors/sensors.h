#ifndef __SENSORS_H__
#define __SENSORS_H__

#define SENSORS_DATA_FILE_VERSION 0x55aa55aa55aa0005

#define MAX_SENSOR_VALUES (3 * 365 * 24)
#define MAX_SENSORS 15

#define ASENSOR_STEP_COUNT       0
#define ASENSOR_PRESSURE         1
#define ASENSOR_TEMPERATURE      2
#define ASENSOR_HUMIDITY         3
#define WEATHER_GOV_TEMPERATURE  4
#define WEATHER_GOV_RELHUMIDITY  5

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
