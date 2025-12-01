#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#include <sdlx.h>
#include <utils.h>

// xxx
// - test at a loc out of US
// - WEXITSTATUS to picoc
// - display last time forecast queried
// - text to speech
//   - "how to access android text to speech class from SDL3"
// - review all prints, do they have progname
// - add api to just update numchars for font

//
// defines
//

#define MAX_FORECAST 100

#define DAILY          0
#define DAY_AND_NIGHT  1
#define HOURLY         2
#define MAX_MODE       3

#define EVID_MODE_SELECT      1
#define EVID_RELOAD_FORECAST  2

#define PARSE_DAILY_FORECAST  1
#define PARSE_HOURLY_FORECAST 2

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
    bool            valid;
    bool            is_daytime;
    char           *day_name;
    char           *icon_url;
    char           *icon_filename;
    sdlx_texture_t *icon_texture;
    char           *short_forecast;
    char           *temperature;
    char           *wind;
    char           *precip;
} forecast_t;

//
// variables
//

char       *progname;
char       *data_dir;
char        icon_dir[100];

int         mode = DAILY;

info_t      info;
forecast_t  daily[MAX_FORECAST];
forecast_t  hourly[MAX_FORECAST];

int         y_top;
int         y_display_begin;
int         y_display_end;

//
// prototypes
//

void cleanup_all(void);
void cleanup_info(void);
void cleanup_forecast(forecast_t *forecast);

bool is_new_forecast_needed(void);
void initiate_forecast_download(void);

int parse_info(void);
void parse_forecast(int which);

void display_forecast(void);

char *get_day_name(char * str);
void split_string(char *str_arg, char **split1, char **split2, int n)

// -----------------  MAIN  ------------------------------------------
    
int main(int argc, char **argv)
{
    int           rc;
    sdlx_event_t  event;
    bool          done = false;
    char          cmd[200];
    char         *mode_str;
    long          timeout;
    bool          daily_forecast_parsed = false;
    bool          hourly_forecast_parsed = false;

    // save args
    if (argc != 2) {
        printf("ERROR %s: data_dir arg expected\n", progname);
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    // construct icon_dir path, and create icon dir
    sprintf(icon_dir, "%s/%s", data_dir, "icon");
    sprintf(cmd, "mkdir -p %s", icon_dir);
    system(cmd);

    // init sdl video subsystem
    rc = sdlx_init(SUBSYS_VIDEO);
    if (rc != 0) {
        printf("ERROR %s: sdlx_init failed\n", progname);
        return 1;
    }

    // init variables that define the display region
    y_display_begin = 100;
    y_display_end   = sdlx_win_height - 200;
    y_top           = y_display_begin;

    // initiate weather forecast download
    if (is_new_forecast_needed()) {
        initiate_forecast_download();
        daily_forecast_parsed = false;
        hourly_forecast_parsed = false;
    }

    // if info.json file is not yet parsed then do so
    if (info.city == NULL) {
        parse_info();
    }

    // init font size and color
    sdlx_print_init(SMALL_FONT, COLOR_WHITE, COLOR_BLACK);

    // runtime loop
    while (!done) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // if forecast is available but not parsed then parse it
        if (!daily_forecast_parsed && util_file_exists(data_dir, "daily.json")) {
            parse_forecast(PARSE_DAILY_FORECAST);
            daily_forecast_parsed = true;
        }
        if (!hourly_forecast_parsed && util_file_exists(data_dir, "hourly.json")) {
            parse_forecast(PARSE_HOURLY_FORECAST);
            hourly_forecast_parsed = true;
        }

        // display forecast         
        if ((mode == DAILY || mode == DAY_AND_NIGHT) && daily_forecast_parsed) {
            display_forecast();
        } else if (mode == HOURLY && hourly_forecast_parsed) {
            display_forecast();
        } else {
            sdlx_print_init(DEFAULT_FONT, COLOR_WHITE, COLOR_BLACK);
            sdlx_render_printf_xyctr(sdlx_win_width/2, sdlx_win_height/2, "Loading");
            sdlx_print_init(SMALL_FONT, COLOR_WHITE, COLOR_BLACK);
        }

        // register events
        mode_str = (mode == DAILY         ? "Day" :
                   (mode == DAY_AND_NIGHT ? "D+N" 
                                          : "Hour"));
        sdlx_register_control_events(
            mode_str, "R", "X", 
            COLOR_WHITE, COLOR_BLACK,
            EVID_MODE_SELECT, EVID_RELOAD_FORECAST, EVID_QUIT);
        sdlx_register_event(NULL, EVID_MOTION);

        // present the display
        sdlx_display_present();

        // wait for event, with xxx timeout
        // xxx timeout on this
        timeout = (!daily_forecast_parsed || !hourly_forecast_parsed) ? 100000 : -1;
        sdlx_get_event(timeout, &event);

        // process events
        switch (event.event_id) {
        case EVID_QUIT:
            done = true;
            break;
        case EVID_RELOAD_FORECAST:
            initiate_forecast_download();
            daily_forecast_parsed = false;
            hourly_forecast_parsed = false;
            break;
        case EVID_MODE_SELECT:
            mode = (mode + 1) % MAX_MODE;
            y_top = y_display_begin;
            break;
        case EVID_MOTION:
            y_top += event.u.motion.yrel;
            if (y_top >= y_display_begin) {
                y_top = y_display_begin;
            }
        }
    }

    // cleanup and end program
    cleanup_all();
    sdlx_quit(SUBSYS_VIDEO);
    printf("INFO %s: terminating\n", progname);
    return 0;
}


void cleanup_all(void)
{
    cleanup_info();
    cleanup_forecast(daily);
    cleanup_forecast(hourly);
}

void cleanup_info(void)
{
    free(info.city);
    free(info.state);
    free(info.forecast_daily_url);
    free(info.forecast_hourly_url);

    memset(&info, 0, sizeof(info));
}

void cleanup_forecast(forecast_t *forecast)
{
    for (int i = 0; i < MAX_FORECAST; i++) {
        free(forecast[i].day_name);
        free(forecast[i].icon_url);
        free(forecast[i].icon_filename);
        sdlx_destroy_texture(forecast[i].icon_texture);
        free(forecast[i].short_forecast);
        free(forecast[i].temperature);
        free(forecast[i].wind);
        free(forecast[i].precip);
    }

    memset(forecast, 0, MAX_FORECAST * sizeof(forecast_t));
}

// -----------------  GET FORECAST JSON FILES  ---------------

#define HEADER "\"User-Agent: (ezApp-Weather, stevenhaid@gmail.com)\""
#define ONE_HOUR 3600

bool is_new_forecast_needed(void)
{
    long tnow, mtime;

    tnow = time(NULL);
    mtime = util_file_mtime(data_dir, "info.json");
    if (mtime == 0 || tnow - mtime > ONE_HOUR) {
        return true;
    }
    mtime = util_file_mtime(data_dir, "daily.json");
    if (mtime == 0 || tnow - mtime > ONE_HOUR) {
        return true;
    }
    mtime = util_file_mtime(data_dir, "hourly.json");
    if (mtime == 0 || tnow - mtime > ONE_HOUR) {
        return true;
    }

    return false;
}

void initiate_forecast_download(void)
{
    char   url[200], cmd[1000];
    char   daily_temp[200], daily_json[200];
    char   hourly_temp[200], hourly_json[200];
    long   start_us;
    int    rc;
    double latitude, longitude;

    printf("INFO %s: initiate_forecast_download starting\n", progname);
    start_us = util_microsec_timer();

    // cleanup forecast data structures
    cleanup_all();

    // delete existing forecast files
    util_delete_file(data_dir, "info.json");
    util_delete_file(data_dir, "daily.json");
    util_delete_file(data_dir, "hourly.json");

    util_delete_file(data_dir, "info.temp");
    util_delete_file(data_dir, "daily.temp");
    util_delete_file(data_dir, "hourly.temp");

    // get lat/long
    util_get_location(&latitude, &longitude, NULL);
    if (latitude == INVALID_NUMBER || longitude == INVALID_NUMBER) {
        printf("ERROR %s: failed to get lat/long\n", progname);
        return;
    }
    printf("INFO %s: long/lat = %0.4f %0.4f\n", progname, latitude, longitude);

    // get forecast information, save to info.json
    sprintf(url, "https://api.weather.gov/points/%0.4f,%0.4f", latitude, longitude);
    sprintf(cmd, "curl --silent --max-time 10 --output %s/%s --header %s %s",
            data_dir, "info.json", HEADER, url);
    printf("INFO %s: RUNNING '%s'\n", progname, cmd);
    rc = system(cmd);
    if (rc != 0) {
        printf("ERROR %s: system curl info.json failed, rc=0x%x\n", progname, rc);  // xxx use WEXITSTATUS
        return;
    }

    // parse file info.json, this will obtain the following, which are used below:
    // - info.forecast_daily_url
    // - info.forecast_hourly_url
    rc = parse_info();
    if (rc != 0) {
        printf("ERROR %s: parse info.json failed\n", progname);
        return;
    }

    // initiate dowload daily forecast, save to daily.json
    sprintf(daily_temp, "%s/daily.temp", data_dir);
    sprintf(daily_json, "%s/daily.json", data_dir);
    sprintf(cmd, "(curl --silent --max-time 30 --output %s --header %s %s; mv %s %s) &",
            daily_temp, HEADER, info.forecast_daily_url, daily_temp, daily_json);
    printf("INFO %s: RUNNING '%s'\n", progname, cmd);
    rc = system(cmd);
    if (rc != 0) {
        printf("ERROR %s: system curl daily.json failed, rc=0x%x\n", progname, rc);
    }

    // initiate dowload hourly forecast, save to hourly.json
    sprintf(hourly_temp, "%s/hourly.temp", data_dir);
    sprintf(hourly_json, "%s/hourly.json", data_dir);
    sprintf(cmd, "(curl --silent --max-time 30 --output %s --header %s %s; mv %s %s) &",
            hourly_temp, HEADER, info.forecast_hourly_url, hourly_temp, hourly_json);
    printf("INFO %s: RUNNING '%s'\n", progname, cmd);
    rc = system(cmd);
    if (rc != 0) {
        printf("ERROR %s: system curl hourly.json failed, rc=0x%x\n", progname, rc);
    }

    // print completed, with duration this routine took
    printf("INFO %s: initiate_forecast_download done, %.1f secs\n", 
           progname, (util_microsec_timer() - start_us) / 1000000.);
}

// -----------------  PARSE JSON FILES  ---------------------------

int parse_info(void)
{
    char         *str = NULL, *end_ptr;
    void         *json = NULL;
    json_value_t *value;
    int           ret = -1, len_ret;

    // read file info.json
    str = util_read_file(data_dir, "info.json", &len_ret);
    if (str == NULL) {
        printf("ERROR %s: parse_info, read info.json, %s\n", progname, strerror(errno));
        goto cleanup_and_return;
    }

    // init json parser
    json = util_json_parse(str, &end_ptr);
    if (json == NULL) {
        printf("ERROR %s: parse_info, parse json\n", progname);
        goto cleanup_and_return;
    }

    // extract needed fields from json
    // - forecast        : daily forecast url
    // - forecastHourly  : hourly forecast url
    // - city, state     : location of the forecast
    value = util_json_get_value(json, "properties", "forecast", NULL);
    if (value->type != JSON_TYPE_STRING) {
        printf("ERROR %s: parse_info, forecast %d\n", progname, value->type);
        goto cleanup_and_return;
    }
    info.forecast_daily_url = strdup(value->u.string);

    value = util_json_get_value(json, "properties", "forecastHourly", NULL);
    if (value->type != JSON_TYPE_STRING) {
        printf("ERROR %s: parse_info, forecastHourly %d\n", progname, value->type);
        goto cleanup_and_return;
    }
    info.forecast_hourly_url = strdup(value->u.string);

    value = util_json_get_value(json, "properties", "relativelocation", "properties", "city", NULL);
    if (value->type != JSON_TYPE_STRING) {
        printf("ERROR %s: parse_info, city %d\n", progname, value->type);
        goto cleanup_and_return;
    }
    info.city = strdup(value->u.string);

    value = util_json_get_value(json, "properties", "relativelocation", "properties", "state", NULL);
    if (value->type != JSON_TYPE_STRING) {
        printf("ERROR %s: parse_info, state %d\n", progname, value->type);
        goto cleanup_and_return;
    }
    info.state = strdup(value->u.string);

    // debug print the results
    printf("INFO %s: parse info.json results:\n", progname);
    printf("INFO %s:   location = %s %s\n", progname, info.city, info.state);
    printf("INFO %s:   daily    = %s\n", progname, info.forecast_daily_url);
    printf("INFO %s:   hourly   = %s\n", progname, info.forecast_hourly_url);

    // success
    ret = 0;

cleanup_and_return:
    util_json_free(json);
    free(str);
    return ret;
}

void parse_forecast(int which)
{
    char *str = NULL;
    void *json = NULL;
    int   len_ret;
    char *end_ptr;
    char *json_filename;
    forecast_t *forecast;
    long start_us;

    // init json_filename, and forecast variables based on 'which' arg
    if (which == PARSE_DAILY_FORECAST) {
        json_filename = "daily.json";
        forecast = daily;
    } else {
        json_filename = "hourly.json";
        forecast = hourly;
    }
    cleanup_forecast(forecast);

    // print starting msg
    printf("INFO %s: parse_forecast %s starting\n", progname, json_filename);
    start_us = util_microsec_timer();

    // read forecast json file
    str = util_read_file(data_dir, json_filename, &len_ret);
    if (str == NULL) {
        printf("ERROR %s: parse_forecast, read forecast json, %s\n", progname, strerror(errno));
        return;
    }

    // init json parser
    json = util_json_parse(str, &end_ptr);
    if (json == NULL) {
        printf("ERROR %s: parse_forecast, parse json\n", progname);
        free(str);
        return;
    }

    // loop over the periods of the json file
    for (int i = 0; i < MAX_FORECAST; i++) {
        forecast_t   *x = &forecast[i];
        json_value_t *value;
        void         *period;
        char          tmp_str[200];
        char          period_str[200];

        // get json period object
        sprintf(period_str, "%d", i);
        value = util_json_get_value(json, "properties", "periods", &period_str, NULL);  // xxx picoc requires &period_str ?
        if (value->type != JSON_TYPE_OBJECT) {
            break;
        }
        period = value->u.object;

        // get is_daytime
        value = util_json_get_value(period, "isDaytime", NULL);
        if (value->type != JSON_TYPE_FLAG) {
            printf("ERROR %s: failed to get isDaytime, %d\n", progname, value->type);
            continue;
        }
        x->is_daytime = value->u.flag;

        // get day_name based on startTime of this period; and
        // also based o the is_daytime flag
        value = util_json_get_value(period, "startTime", NULL);
        if (value->type != JSON_TYPE_STRING) {
            printf("ERROR %s: failed to get startTime, %d\n", progname, value->type);
            continue;
        }
        if (which == PARSE_DAILY_FORECAST) {
            if (x->is_daytime) {
                sprintf(tmp_str, "%s", get_day_name(value->u.string));
            } else {
                sprintf(tmp_str, "%s Night", get_day_name(value->u.string));
            }
        } else {
            int hour;
            sprintf(tmp_str, "%s", get_day_name(value->u.string));
            sscanf(value->u.string+10, "T%d", &hour);
            char *ampm = hour < 12 ? "AM" : "PM";
            if (hour > 12) hour -= 12;
            if (hour == 0) hour = 12;
            sprintf(tmp_str, "%s %d%s", get_day_name(value->u.string), hour, ampm);
        }
        x->day_name = strdup(tmp_str);
        
        // get icon_url
        value = util_json_get_value(period, "icon", NULL);
        if (value->type != JSON_TYPE_STRING) {
            printf("ERROR %s: failed to get icon, %d\n", progname, value->type);
            continue;
        }
        x->icon_url = strdup(value->u.string);

        // create icon filename from icon url
        // url example: https://api.weather.gov/icons/land/day/few?size=medium
        // - remove leading https://api.weather.gov/
        // - replace '/' chars with '-'
        // - append ".png"
        if (strncmp(x->icon_url, "https://api.weather.gov/", 24) != 0) {
            printf("ERROR %s: unexpected icon_url %s\n", progname, x->icon_url);
            continue;
        } else {
            strcpy(tmp_str, x->icon_url+24);
            for (int ii = 0; tmp_str[ii]; ii++) {
                if (tmp_str[ii] == '/') tmp_str[ii] = '-';
            }
            strcat(tmp_str, ".png");
            x->icon_filename = strdup(tmp_str);
        }

        // get shortForecast
        value = util_json_get_value(period, "shortForecast", NULL);
        if (value->type != JSON_TYPE_STRING) {
            printf("ERROR %s: failed to get shortForecast, %d\n", progname, value->type);
            continue;
        }
        x->short_forecast = strdup(value->u.string);

        // get temperature, and append temperatureUnit
        value = util_json_get_value(period, "temperature", NULL);
        if (value->type != JSON_TYPE_NUMBER) {
            printf("ERROR %s: failed to get temperature, %d\n", progname, value->type);
            continue;
        }
        double temperature = value->u.number;
        value = util_json_get_value(period, "temperatureUnit", NULL);
        if (value->type != JSON_TYPE_STRING) {
            printf("ERROR %s: failed to get temperatureUnit, %d\n", progname, value->type);
            continue;
        }
        sprintf(tmp_str, "%.0f%s", temperature, value->u.string);
        x->temperature = strdup(tmp_str);

        // get wind speed and direction
        int cnt, low, high;
        char wind_speed[40], wind_dir[40];

        value = util_json_get_value(period, "windSpeed", NULL);
        if (value->type != JSON_TYPE_STRING) {
            printf("ERROR %s: failed to get windSpeed, %d\n", progname, value->type);
            continue;
        }
        strcpy(wind_speed, value->u.string);

        value = util_json_get_value(period, "windDirection", NULL);
        if (value->type != JSON_TYPE_STRING) {
            printf("ERROR %s: failed to get windDirection, %d\n", progname, value->type);
            continue;
        }
        strcpy(wind_dir, value->u.string);

        cnt = sscanf(wind_speed, "%d to %d mph", &low, &high);
        if (cnt == 1) {
            sprintf(tmp_str, "%d%s", low, wind_dir);
        } else if (cnt == 2) {
            sprintf(tmp_str, "%d-%d%s", low, high, wind_dir);
        } else {
            tmp_str[0] = '\0';
        }

        x->wind = strdup(tmp_str);

        // get precip probability
        value = util_json_get_value(period, "probabilityOfPrecipitation", "value", NULL);
        if (value->type != JSON_TYPE_NUMBER) {
            printf("ERROR %s: failed to get probabilityOfPrecipitation, %d\n", progname, value->type);
            continue;
        }
        sprintf(tmp_str, "%.0f%%", value->u.number);
        x->precip = strdup(tmp_str);

        // set forecast item valid
        x->valid = true;

        // debug print forecast info
        if (1) {
            printf("INFO %s: %s periods[%s] ...\n", progname, json_filename, period_str);
            printf("INFO %s:   is_daytime     %d\n", progname, x->is_daytime);
            printf("INFO %s:   day_name       %s\n", progname, x->day_name);
            printf("INFO %s:   icon_url       %s\n", progname, x->icon_url);
            printf("INFO %s:   icon_filename  %s\n", progname, x->icon_filename);
            printf("INFO %s:   short_forecast %s\n", progname, x->short_forecast);
            printf("INFO %s:   temperature    %s\n", progname, x->temperature);
            printf("INFO %s:   wind           %s\n", progname, x->wind);
            printf("INFO %s:   precip         %s\n", progname, x->precip);
        }
    }

    // download icons that have not already been dowloaded
    for (int i = 0; i < MAX_FORECAST; i++) {
        forecast_t *x = &forecast[i];
        char cmd[500];

        if (!x->valid) {
            continue;
        }

        if (!util_file_exists(icon_dir, x->icon_filename)) {
            sprintf(cmd, "curl --silent --max-time 10 --output %s/%s --header %s %s",
                    icon_dir, x->icon_filename, HEADER, x->icon_url);
            printf("INFO %s: %s\n", progname, cmd);
            system(cmd);
        }
    }

    // create icon_texture for each forecast element
    for (int i = 0; i < MAX_FORECAST; i++) {
        forecast_t        *x = &forecast[i];
        unsigned char  *pixels = NULL;
        int             rc, w, h;
        sdlx_texture_t *icon_texture;

        if (!x->valid) {
            continue;
        }

        if (util_file_exists(icon_dir, x->icon_filename)) {
            rc = util_read_png_file(icon_dir, x->icon_filename, &pixels, &w, &h);
            if (rc != 0) {
                printf("ERROR %s failed to decode png file %s\n", progname, x->icon_filename);
            } else {
                icon_texture = sdlx_create_texture_from_pixels(pixels, w, h);
                if (icon_texture == NULL) {
                    printf("ERROR %s failed to create icon_texture\n", progname);
                } else {
                    x->icon_texture = icon_texture;
                }
                free(pixels);
            }
        }
    }

    // cleanup and return
    printf("INFO %s: parse_forecast %s completed, %0.3f secs\n", 
           progname, json_filename, 
           (util_microsec_timer() - start_us) / 1000000.0);
    util_json_free(json);
    free(str);
}

// -----------------  DISPLAY DAILY FORECAST  ----------------

#define ICON_WH 200  // width and height of forecast icon
#define Y_STEP  250

void display_forecast(void)
{
    int y;

    y = y_top;

    sdlx_render_printf_xyctr(sdlx_win_width/2, y, "%s %s", info.city, info.state);
    y += sdlx_char_height;

    for (int i = 0; i < MAX_FORECAST; i++) {
        forecast_t *x = (mode == HOURLY ? &hourly[i] : &daily[i]);

        // if forecast entry is not valid then continue
        if (!x->valid) {
            continue;
        }        

        // skip nightime forecast when mode is DAILY
        if (mode == DAILY && !x->is_daytime) {
            continue;
        }

        // if y coord is below bottom then done displaying
        if (y > y_display_end - ICON_WH) {
            break;
        }

        // if y coord is above top then skip
        if (y < y_display_begin - ICON_WH) {
            y += Y_STEP;
            continue;
        }

        // display the forecast icon
        if (x->icon_texture) {
            sdlx_render_texture(0, y, ICON_WH, ICON_WH, x->icon_texture);
        }

        // split the short forecast string to 2 lines
        split_string(x->short_forecast, &sf1, &sf2, 24);

        // display forecast info:
        // - day_name, temperature, wind, and precip probability
        sdlx_render_printf(ICON_WH,y, "%s: %s %s %s", x->day_name, x->temperature, x->wind, x->precip);
        sdlx_render_printf(ICON_WH,y+1*sdlx_char_height, "%s", sf1);
        sdlx_render_printf(ICON_WH,y+2*sdlx_char_height, "%s", sf2);

        // advance y
        y += Y_STEP;
    }
}

// -----------------  UTILS  ----------------------------------

#ifdef LINUX
extern char *strptime(const char *s, const char *format, struct tm *tm);
#endif

char *get_day_name(char * time_str)
{
    time_t t;
    struct tm tm;
    static char day_name_str[20];

    memset(&tm, 0, sizeof(tm));
    if (strptime(time_str, "%Y-%m-%d", &tm) != NULL) {
        t = mktime(&tm);
        if (t > 0) {
            strftime(day_name_str, sizeof(day_name_str), "%a", &tm);
            return day_name_str;
        } else {
            return "Error";
        }
    }

    return "Error";
}

void split_string(char *str_arg, char **split1, char **split2, int n)
{
    int k;
    static char str[200];

    // if str_arg is null then return empty str for the 2 splits
    if (str_arg == NULL) {
        **split1 = "";;
        **split2 = "";
        return;
    }

    // make static copy of caller str_arg
    strcpy(str, str_arg);

    // if str is already <= n chars then no need to split
    if (strlen(str) <= n) {
        **split1 = str;
        **split2 = "";
        return;
    }

    // find first space char at or below str idx n
    for (k = n; k > 0; k--) {
        if (str[k] == ' ') {
            break;
        }
    }

    // if space char found then split str at the space char
    if (k > 0) {
        str[k] = '\0';
        **split1 = str;
        **split2 = &str[k+1];
        return;
    }

    // no space char found, return the entire str in split1
    *split1 = str;
    *split2 = "";
}
