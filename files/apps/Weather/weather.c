#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>
//#include <libgen.h>

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

#define DAILY          0
#define DAY_AND_NIGHT  1
#define HOURLY         2

#define MAX_MODE 3

#define MAX_DAILY 20

#define EVID_MODE_SELECT 1

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
    char *icon_url;
    char *icon_filename;
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
char  icon_dir[100];

info_t  info;
daily_t daily[MAX_DAILY];
int     max_daily;

int     y_top;
int     y_display_begin;
int     y_display_end;

int     mode;

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
    char         cmd[200];
    char         *mode_str;

    // save args
    if (argc != 2) {
        printf("ERROR: data_dir arg expected\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);
    
    // construct icon_dir path, and create icon dir
    sprintf(icon_dir, "%s/%s", data_dir, "icon");
    sprintf(cmd, "mkdir -p %s", icon_dir);
    system(cmd);

    // xxx
    mode = util_get_int_param(data_dir, "mode", DAILY);

    // init sdl video subsystem
    rc = sdlx_init(SUBSYS_VIDEO);
    if (rc != 0) {
        printf("ERROR %s: sdlx_init failed\n", progname);
        return 1;
    }

    //rc = system("ls");
    //printf("rc = %x\n", rc);
    //return 0;
    y_display_begin = 100;
    y_display_end   = sdlx_win_height - 200;
    y_top           = y_display_begin;

    // get weather forecast
    rc = get_weather_forecast();
    if (rc != 0) {
        printf("ERROR %s: get_weather_forecast failed\n", progname);
        return 1;
    }


    // init font size and color
    sdlx_print_init(SMALL_FONT, COLOR_WHITE, COLOR_BLACK);

    // runtime loop
    while (!done) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);


        // xxx del
        // display 'Hello' at center of display
        // sdlx_render_printf_xyctr(sdlx_win_width/2, sdlx_win_height/2, "Hello");

        if (mode == DAILY || mode == DAY_AND_NIGHT) {
            display_daily_forecast();
        } else {
            display_hourly_forecast();
        }

        // register control events
        mode_str = (mode == DAILY         ? "Daily" :
                   (mode == DAY_AND_NIGHT ? "Day+Night" 
                                          : "Hourly"));
        sdlx_register_control_events(mode_str, NULL, "X", COLOR_WHITE, COLOR_BLACK, EVID_MODE_SELECT, 0, EVID_QUIT);

        // register for events
        sdlx_register_event(NULL, EVID_MOTION);

        // present the display
        sdlx_display_present();

        // wait for event, with infinite timeout
        sdlx_get_event(-1, &event);

        // process events
        switch (event.event_id) {
        case EVID_QUIT:
            done = true;
            break;
        case EVID_MODE_SELECT:
            mode = (mode + 1) % MAX_MODE;
            break;
        case EVID_MOTION:
            y_top += event.u.motion.yrel;
            if (y_top >= y_display_begin) {
                y_top = y_display_begin;
            }

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
#define ONE_HOUR 3600

int parse_info(void);
int parse_daily(void);

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
        if (mtime == 0 || tnow - mtime > ONE_HOUR) {
            get_new_forecast = true;
            break;
        }
        mtime = util_file_mtime(data_dir, "daily.json");
        if (mtime == 0 || tnow - mtime > ONE_HOUR) {
            get_new_forecast = true;
            break;
        }
        //mtime = util_file_mtime(data_dir, "hourly.json");
        //if (mtime == 0 || tnow - mtime > ONE_HOUR) {
        //    get_new_forecast = true;
        //    break;
        //}
    } while (0);

    // xxx temp
    //get_new_forecast = false;
    printf("INFO: get_new_forecast = %d\n", get_new_forecast);

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
    rc = parse_daily();
    if (rc != 0) {
        printf("ERROR: parse daily.json failed\n");
        return -1;
    }

    return 0;
}

int parse_info(void)
{
    char *str=NULL, *end_ptr;
    json_value_t *value;
    void *json=NULL;
    int ret = -1, len_ret;

    if (info.city != NULL) {
        return 0;
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

// Sample daily forecast ...
//  "number": 1,
//  "name": "Thanksgiving Day",
//  "startTime": "2025-11-27T06:00:00-05:00",
//  "endTime": "2025-11-27T18:00:00-05:00",
//  "isDaytime": true,
//  "temperature": 44,
//  "temperatureUnit": "F",
//  "temperatureTrend": null,
//  "probabilityOfPrecipitation": {
//      "unitCode": "wmoUnit:percent",
//      "value": 1
//  },
//  "windSpeed": "7 to 10 mph",
//  "windDirection": "SW",
//  "icon": "https://api.weather.gov/icon/land/day/sct?size=medium",
//  "shortForecast": "Mostly Sunny",
//  "detailedForecast": "Mostly sunny. ..."

// typedef struct {
// k   char *day_name;
// k   bool  is_daytime;
// k   char *icon_url;
// k   char *icon_filename;
// k   char *short_forecast;
// k   char *temperature;
//     char *wind;
//     int   precip;
// } daily_t;

int parse_daily(void)
{
    char *str = NULL;
    void *json = NULL;
    int   ret = -1, len_ret;
    char *end_ptr;

    str = util_read_file(data_dir, "daily.json", &len_ret);
    if (str == NULL) {
        printf("ERROR: parse_daily, read daily.json, %s\n", strerror(errno));
        goto cleanup_and_return;
    }

    json = util_json_parse(str, &end_ptr);
    if (json == NULL) {
        printf("ERROR: parse_daily, parse json\n");
        goto cleanup_and_return;
    }
    printf("BACK FROM json parse %p\n", json);

    for (max_daily = 0; max_daily < MAX_DAILY; max_daily++) {
        daily_t      *x = &daily[max_daily];
        json_value_t *value;
        void         *period;
        char          tmp_str[200];

        // get periods[max_daily]
        sprintf(tmp_str, "%d", max_daily);
        printf("BEFORE PERIOD json=%p  %d %s = %p\n", json, max_daily, tmp_str, period);
        value = util_json_get_value(json, "properties", "periods", &tmp_str, NULL);  // xxx picoc requires &tmp_str
        if (value->type != JSON_TYPE_OBJECT) {
            printf("value type %d\n", value->type);
            break;
        }
        period = value->u.object;
        printf("PERIOD %d %s = %p\n", max_daily, tmp_str, period);

        // get day_name
        value = util_json_get_value(period, "name", NULL);
        if (value->type != JSON_TYPE_STRING) {
            printf("failed to get name, %d\n", value->type);
            break;
        }
        x->day_name = strdup(value->u.string);
        printf("DAY_NAME %s\n", x->day_name);

        // get is_daytime
        value = util_json_get_value(period, "isDaytime", NULL);
        if (value->type != JSON_TYPE_FLAG) {
            printf("failed to get isDaytime, %d\n", value->type);
            break;  // xxx maybe just continue on errors
        }
        x->is_daytime = value->u.flag;
        printf("IS_DAYTIME %d\n", x->is_daytime);

        // get temperature
        value = util_json_get_value(period, "temperature", NULL);
        if (value->type != JSON_TYPE_NUMBER) {
            printf("failed to get temperature, %d\n", value->type);
            break;
        }
        double temperature = value->u.number;
        value = util_json_get_value(period, "temperatureUnit", NULL);
        if (value->type != JSON_TYPE_STRING) {
            printf("failed to get temperatureUnit, %d\n", value->type);
            break;
        }
        sprintf(tmp_str, "%.0f %s", temperature, value->u.string);
        x->temperature = strdup(tmp_str);
        printf("TEMPERATURE %s\n", x->temperature);

        // get icon url and filename
        value = util_json_get_value(period, "icon", NULL);
        if (value->type != JSON_TYPE_STRING) {
            printf("failed to get icon, %d\n", value->type);
            break;
        }
        x->icon_url = strdup(value->u.string);
        printf("ICON_URL      %s\n", x->icon_url);

        // create icon filename from icon url
        // url example: https://api.weather.gov/icons/land/day/few?size=medium
        // - remove leading https://api.weather.gov/
        // - replace '/' chars with '-'
        // - append ".png"
        if (strncmp(x->icon_url, "https://api.weather.gov/", 24) != 0) {
            printf("ERROR %s: unexpected icon_url %s\n", progname, x->icon_url);
        } else {
            strcpy(tmp_str, x->icon_url+24);
            for (int i = 0; tmp_str[i]; i++) {
                if (tmp_str[i] == '/') tmp_str[i] = '-';
            }
            strcat(tmp_str, ".png");
            x->icon_filename = strdup(tmp_str);
        }
        printf("ICON_FILENAME %s\n", x->icon_filename);

        // get shortForecast
        value = util_json_get_value(period, "shortForecast", NULL);
        if (value->type != JSON_TYPE_STRING) {
            printf("failed to get shortForecast, %d\n", value->type);
            break;
        }
        x->short_forecast = strdup(value->u.string);
        printf("SHORT_FORECAST      %s\n", x->short_forecast);
    }
    printf("MAX_DAILY = %d\n", max_daily);

    // download icons that have not already been dowloaded
    for (int i = 0; i < max_daily; i++) {
        daily_t *x = &daily[i];
        char cmd[500];

        if (util_file_exists(icon_dir, x->icon_filename)) {
            printf("EXISTS %s\n", x->icon_filename);
        } else {
            sprintf(cmd, "curl --silent --max-time 10 --output %s/%s --header %s %s",
                    icon_dir, x->icon_filename, HEADER, x->icon_url);
            printf("ICON DOWNLOAD CMD %s\n", cmd);
            system(cmd);
        }
    }

    printf("RETURNING\n");
    ret = 0;

cleanup_and_return:
    util_json_free(json);
    free(str);
    return ret;
}

// -----------------  DISPLAY DAILY FORECAST  ----------------

void display_daily_forecast(void)
{
    int rc, i, w, h, y;
    sdlx_texture_t *icon_texture;
    unsigned char *pixels;

    y = y_top;

    for (i = 0; i < max_daily; i++) {
        daily_t *x = &daily[i];

        if (mode == DAILY && !x->is_daytime) {
            continue;
        }

        if (y > y_display_end - 200) {
            printf("DONE AT y = %d  y_display_end = %d\n", y, y_display_end);
            break;
        }

        if (y < y_display_begin - 200) {
            printf("CONTINUING AT y = %d  begin=%d\n", y, y_display_begin);
            y += 250;
            continue;
        }

        printf("displaying at y = %d\n", y);

        do {
            if (x->icon_filename == NULL) {
                printf("ERROR %s: icon_filename is NULL\n", progname);
                break;
            }

            rc = util_read_png_file(icon_dir, x->icon_filename, &pixels, &w, &h);
            if (rc != 0) {
                printf("ERROR %s failed to decode png file %s\n", progname, x->icon_filename);
                break;
            }

            icon_texture = sdlx_create_texture_from_pixels(pixels, w, h);
            printf("texture w,h %d %d\n", w,h);
            if (icon_texture == NULL) {
                free(pixels);
                printf("ERROR %s failed to create icon_texture\n", progname);
                break;
            }

            sdlx_render_texture(0,y,200,200, icon_texture);

            free(pixels);
            pixels = NULL;
        } while (0);

//123456789 123456789 123456789 
//Slight chance Light RainXxxxxxxxxxxxxx
// xxx what if no short_forecast
        char *sfl1=NULL, *sfl2=NULL;
        char short_forecast[200];
        strcpy(short_forecast, x->short_forecast);
        if (strlen(short_forecast) <= 24) {
            sfl1 = short_forecast;
            sfl2 = NULL;
        } else {
            int k;
            for (k = 24; k > 0; k--) {
                if (short_forecast[k] == ' ') {
                    break;
                }
            }
            printf("XXX '%s' k=%d\n", short_forecast, k);
            if (k > 0) {
                short_forecast[k] = '\0';
                sfl1 = short_forecast;
                sfl2 = (short_forecast[k+1] != '\0' ? &short_forecast[k+1] : NULL);
            } else {
                sfl1 = short_forecast;
                sfl2 = NULL;
            }
        }

        sdlx_render_printf(200,y, "%s: %s", x->day_name, x->temperature);
        sdlx_render_printf(200,y+1*sdlx_char_height, "%s", sfl1);
        if (sfl2)
            sdlx_render_printf(200,y+2*sdlx_char_height, "%s", sfl2);

        y += 250;
    }
}

// -----------------  DISPLAY HOURLY FORECAST  ----------------

void display_hourly_forecast(void)
{
}
