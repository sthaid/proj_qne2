#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <utils.h>
#include <svcs/Sensors/common.h>

// xxx todo
// - get lat/long

// references:
// - https://www.weather.gov/documentation
// - https://www.weather.gov/documentation/services-web-api

// defines
#define INVALID_NUMBER 99999999  // xxx move to common place

// prototypes
static int run_curl(char *url, char *filename);
static void *get_json_root(char *filename);
static char *get_hourly_forecast_url(double latitude, double longitude);

// -----------------  PUBLIC ROUTINES  -----------------------------

void get_weather(double *temperature, double *humidity)
{
    void *root;
    json_value_t *value;
    char *hourly_forecast_url;
    int ret;

    // preset return values to invalid
    *temperature = INVALID_NUMBER;
    *humidity    = INVALID_NUMBER;

    // get the url to obtain hourly forecast for specified lat/long
    hourly_forecast_url = get_hourly_forecast_url(42.4334, -71.6078);
    if (hourly_forecast_url == NULL) {
        printf("ERROR %s: get_hourly_forecast_url failed\n", progname);
        return;
    }

    // run curl to get the hourly forecast json copied to file curl.out
    ret = run_curl(hourly_forecast_url, "curl.out");
    if (ret != 0) {
        printf("ERROR %s: run_curl get_hourly_forecast failed\n", progname);
        return;
    }

    // parse the json in file curl.out
    root = get_json_root("curl.out");
    if (root == NULL) {
        printf("ERROR %s: parse hourly forecast json failed\n", progname);
        return;
    }

    // get the first (most recent) forecast values for temperature and humidity
    value = util_json_get_value(root, "properties", "periods", "0", "temperature", NULL);
    if (value->type == JSON_TYPE_NUMBER) {
        *temperature = value->u.number;
    }
    value = util_json_get_value(root, "properties", "periods", "0", "relativeHumidity", "value", NULL);
    if (value->type == JSON_TYPE_NUMBER) {
        *humidity = value->u.number;
    }

    // free the parsed json
    util_json_free(root);
}

// -----------------  SUPPORT ROUTINES  ----------------------------

static char *get_hourly_forecast_url(double latitude, double longitude)
{
    char          curl_url[200];
    void         *root = NULL;
    json_value_t *value;
    int           ret = -1;

    static char *hourly_forecast_url;

    sprintf(curl_url, "\"https://api.weather.gov/points/%0.4f,%0.4f\"", latitude, longitude);
    ret = run_curl(curl_url, "curl.out");
    if (ret != 0) {
        printf("ERROR %s: run_curl get_hourly_forecast_url failed\n", progname);
        goto done; 
    }

    root = get_json_root("curl.out");
    if (root == NULL) {
        printf("ERROR %s: json parse get_hourly_forcast_url failed\n", progname);
        goto done;
    }

    value = util_json_get_value(root, "properties", "forecastHourly", NULL);
    if (value->type != JSON_TYPE_STRING) {
        printf("ERROR %s: json hourly_forecast_url value is not string, %d\n", progname, value->type);
        goto done;
    }

    free(hourly_forecast_url);
    hourly_forecast_url = strdup(value->u.string);
    printf("INFO %s obtained hourly_forecast_url = '%s'\n", progname, hourly_forecast_url);
    ret = 0;

done:
    util_json_free(root);
    return hourly_forecast_url;
}

static int run_curl(char *url, char *filename)
{
    char cmd[500];
    int  ret;

    //printf("INFO %s: url = %s\n", progname, url);

    util_delete_file(data_dir, filename);

    sprintf(cmd, "curl -s %s > %s/%s", url, data_dir, "curl.out");

    ret = system(cmd);
    if (ret != 0) {
        printf("ERROR %s: curl failed for url '%s'\n", progname, url);
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
        printf("ERROR %s; failed to read file %s/%s, %s\n", 
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

