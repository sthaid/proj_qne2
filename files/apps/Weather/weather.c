#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#include <sdlx.h>
#include <utils.h>

// xxx testing
// - at a info out of US
// - at PeutoRico

// xxx events
// daily
// hourly
// vertical motion
// add WEXITSTATUS to picoc

//
// defines
//

#define DAILY  1
#define HOURLY 2

#define MAX_DAILY 20

//
// typedefs
//

typedef struct {
    char *city;
    char *state;
    char *forecast_daily_url;
    char *forecast_hourly_url;
} info_t;

typedef struct {
    char *day_name;
    bool  is_daytime;
    char *icon;
    char *short_forecast;
    char *temperature;
    char *wind;
    int   precip;
} daily_t;

//
// variables
//

char *progname;
char *data_dir;

info_t  info;
daily_t daily[MAX_DAILY];
int     max_daily;

//
// prototypes
//

int get_weather_forecast(void);
void display_daily_forecast(void);
void display_hourly_forecast(void);

// -----------------  MAIN  ------------------------------------------
    
int main(int argc, char **argv)
{
    int          rc;
    sdlx_event_t event;
    bool         done = false;
    int          mode = DAILY;

    // save args
    if (argc != 2) {
        printf("ERROR: data_dir arg expected\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    //rc = system("ls");
    //printf("rc = %x\n", rc);
    //return 0;

    // get weather forecast
    rc = get_weather_forecast();
    printf("XXX rc %d\n", rc);
    return 0;

    // init sdl video subsystem
    rc = sdlx_init(SUBSYS_VIDEO);
    if (rc != 0) {
        printf("ERROR %s: sdlx_init failed\n", progname);
        return 1;
    }

    // init font size and color
    sdlx_print_init(DEFAULT_FONT, COLOR_WHITE, COLOR_BLACK);

    // runtime loop
    while (!done) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // register control event to
        // - end program
        sdlx_register_control_events(NULL, NULL, "X", COLOR_WHITE, COLOR_BLACK, 0, 0, EVID_QUIT);

        // xxx del
        // display 'Hello' at center of display
        // sdlx_render_printf_xyctr(sdlx_win_width/2, sdlx_win_height/2, "Hello");

        if (mode == DAILY) {
            display_daily_forecast();
        } else {
            display_hourly_forecast();
        }

        // present the display
        sdlx_display_present();

        // wait for event, with infinite timeout
        sdlx_get_event(-1, &event);

        // process events
        switch (event.event_id) {
        case EVID_QUIT:
            done = true;
            break;
        }
    }

    // xxx free all strdups too

    // cleanup and end program
    sdlx_quit(SUBSYS_VIDEO);
    printf("INFO %s: terminating\n", progname);
    return 0;
}

// -----------------  GET WEATHER FORECAST  ------------------

#define HEADER "\"User-Agent: (ezApp-Weather, stevenhaid@gmail.com)\""
#define TWO_HOURS 7200

int parse_info(void);

int get_weather_forecast(void)
{
    time_t tnow, mtime;
    bool get_new_forecast;
    double latitude, longitude;
    char url[200], cmd[400];
    int rc;

    // if the weather forecast data files (points, daily, hourly) all exist, and
    // are all less than 2 hours old then  ..

    get_new_forecast = false;
    do {
        tnow = time(NULL);
        mtime = util_file_mtime(data_dir, "info.json");
        if (mtime == 0 || tnow - mtime > TWO_HOURS) {
            get_new_forecast = true;
            break;
        }
        mtime = util_file_mtime(data_dir, "daily.json");
        if (mtime == 0 || tnow - mtime > TWO_HOURS) {
            get_new_forecast = true;
            break;
        }
        mtime = util_file_mtime(data_dir, "hourly.json");
        if (mtime == 0 || tnow - mtime > TWO_HOURS) {
            get_new_forecast = true;
            break;
        }
    } while (0);

    // xxx temp
    get_new_forecast = false;

    if (get_new_forecast) {
        printf("INFO: getting new forecast\n");

        // delete existing forecast files
        util_delete_file(data_dir, "info.json");
        util_delete_file(data_dir, "daily.json");
        util_delete_file(data_dir, "hourly.json");

        // get lat/long
        util_get_location(&latitude, &longitude, NULL);  // xxx check for no lat/long
        printf("INFO: lat/long = %0.4f %0.4f\n", latitude, longitude);

        // get forecast information, save to info.json
        sprintf(url, "https://api.weather.gov/points/%0.4f,%0.4f", latitude, longitude);
        sprintf(cmd, "curl --silent --max-time 10 --output %s/%s --header %s %s",
                data_dir, "info.json", HEADER, url);
        rc = system(cmd);
        if (rc != 0) {
            printf("ERROR: failed '%s', rc=0x%x\n", cmd, rc);
            return -1;
        }

        // parse file info.json
        rc = parse_info();
        if (rc != 0) {
            printf("ERROR: parse info.json failed\n");
            return -1;
        }

        // get daily forecast, save to daily.json
        sprintf(cmd, "curl --silent --max-time 10 --output %s/%s --header %s %s",
                data_dir, "daily.json", HEADER, info.forecast_daily_url);
        rc = system(cmd);
        if (rc != 0) {
            printf("ERROR: failed '%s', rc=0x%x\n", cmd, rc);
            return -1;
        }
    }

    // parse info_json
    rc = parse_info();
    if (rc != 0) {
        printf("ERROR: parse info.json failed\n");
        return -1;
    }

    // parse daily_json

    // download icons that have not already been dowloaded

    return 0;
}

int parse_info(void)
{
    char *str=NULL, *end_ptr;
    json_value_t *value;
    void *json=NULL;
    int ret = -1, len_ret;

    if (info.city[0] != '\0') {
        return;
    }

    str = util_read_file(data_dir, "info.json", &len_ret);
    if (str == NULL) {
        printf("ERROR: parse_info, read info.json, %s\n", strerror(errno));
        goto cleanup_and_return;
    }

    json = util_json_parse(str, &end_ptr);
    if (json == NULL) {
        printf("ERROR: parse_info, parse json\n");
        goto cleanup_and_return;
    }

    value = util_json_get_value(json, "properties", "forecast", NULL);
    if (value->type != JSON_TYPE_STRING) {
        printf("ERROR: parse_info, forecast %d\n", value->type);
        goto cleanup_and_return;
    }
    info.forecast_daily_url = strdup(value->u.string);

    value = util_json_get_value(json, "properties", "relativelocation", "properties", "city", NULL);
    if (value->type != JSON_TYPE_STRING) {
        printf("ERROR: parse_info, city %d\n", value->type);
        goto cleanup_and_return;
    }
    info.city = strdup(value->u.string);

    value = util_json_get_value(json, "properties", "relativelocation", "properties", "state", NULL);
    if (value->type != JSON_TYPE_STRING) {
        printf("ERROR: parse_info, state %d\n", value->type);
        goto cleanup_and_return;
    }
    info.state = strdup(value->u.string);

    printf("INFO: parse_info: city=%s state=%s\n", info.city, info.state);
    printf("INFO: parse_info: daily = %s\n", info.forecast_daily_url);

    ret = 0;

cleanup_and_return:
    util_json_free(json);
    free(str);
    return ret;
}

// -----------------  DISPLAY DAILY FORECAST  ----------------

void display_daily_forecast(void)
{
}

// -----------------  DISPLAY HOURLY FORECAST  ----------------

void display_hourly_forecast(void)
{
}
