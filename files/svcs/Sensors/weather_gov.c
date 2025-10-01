#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <utils.h>
#include <svcs/Sensors/common.h>

// xxx todo
// - if lat/long change
// - get lat/long
// - comments
// - retries in get_hourly_forecast
// - check rc throughout

// defines
#define INVALID_NUMBER 99999999  // xxx move to common place

// variables
static char *hourly_forecast_url;

// prototypes
static int run_curl(char *url, char *filename);
static void *get_json_root(char *filename);
static int get_hourly_forecast_url(double latitude, double longitude);

// -----------------  PUBLIC ROUTINES  -----------------------------

int get_weather(double *temperature, double *humidity)
{
    void *root;
    json_value_t *value;

    *temperature = INVALID_NUMBER;
    *humidity    = INVALID_NUMBER;

    if (hourly_forecast_url == NULL) {
        get_hourly_forecast_url(42.4334, -71.6078);  // xxx lat/long
        if (hourly_forecast_url == NULL) {
            // xxx if err
        }
    }

    run_curl(hourly_forecast_url, "curl.out");
    // xxx check ret

    root = get_json_root("curl.out");

    value = util_json_get_value(root, "properties", "periods", "0", "temperature", NULL);
    if (value->type == JSON_TYPE_NUMBER) {
        *temperature = value->u.number;
    }

    value = util_json_get_value(root, "properties", "periods", "0", "relativeHumidity", "value", NULL);
    if (value->type == JSON_TYPE_NUMBER) {
        *humidity = value->u.number;
    }

    util_json_free(root);

    return 0;
}

// -----------------  SUPPORT ROUTINES  ----------------------------

static int get_hourly_forecast_url(double latitude, double longitude)
{
    char          cmd[200];  // xxx rename
    char         *extra_header;
    void         *root = NULL;
    json_value_t *value;
    int           ret = -1;

    extra_header = "User-Agent: ezapp-app (stevenhaid@gmail.com)";
    sprintf(cmd, "\"https://api.weather.gov/points/%0.4f,%0.4f\" -H \"%s\"",
            latitude, longitude, extra_header);
    ret = run_curl(cmd, "curl.out");
    if (ret != 0) {
        goto done; 
    }

    root = get_json_root("curl.out");
    if (root == NULL) {
        ret = -1;
        goto done;
    }

    value = util_json_get_value(root, "properties", "forecastHourly", NULL);
    if (value->type != JSON_TYPE_STRING) {
        printf("ERROR %s: json value is not string, %d\n", progname, value->type);
        ret = -1;
        goto done;
    }

    hourly_forecast_url = strdup(value->u.string);
    printf("INFO %s hourly_forecast_url = '%s'\n", progname, hourly_forecast_url);
    ret = 0;

done:
    util_json_free(root);
    return ret;
}

static int run_curl(char *url, char *filename)
{
    char cmd[500];
    int  ret;

    printf("INFO %s: url = %s\n", progname, url);
    util_delete_file(data_dir, filename);

    sprintf(cmd, "curl %s > %s/%s", url, data_dir, "curl.out");
    printf("%s\n", cmd);

    ret = system(cmd);
    if (ret != 0) {
        return -1;
    }

    return 0;
}

static void *get_json_root(char *filename)
{
    char *str;
    void *root;
    int   len;

    str = util_read_file(data_dir, filename, &len);
    if (str == NULL) {
        printf("ERROR %s; failed to read %s/%s, %s\n", 
               progname, data_dir, filename, strerror(errno));
        return NULL;
    }

    root = util_json_parse(str);
    if (root == NULL) {
        printf("ERROR %s; failed to parse json\n", progname);
        free(str);
        return NULL;
    }

    free(str);

    return root;
}

