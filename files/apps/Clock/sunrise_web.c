#include <stdio.h>  // xxx
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
    
#include <sdlx.h> //xxx
#include <utils.h>

#include "apps/Clock/common.h"

//static void make_local_time_str(char *str_in, char *str_out, char *debug);
static int run_curl(char *url, char *filename);
static void *get_json_root(char *filename);

void sunrise_sunset_web(char *sunrise, char *sunset, char *midday)
{
    char         curl_url[200];
    json_value_t rise, set;
    int          ret, cnt;
    double       latitude, longitude;
    void        *root = NULL;
    struct tm    tm;
    time_t       t_rise_gm, t_set_gm, t_midday_gm;

    // preset return string values
    strcpy(sunrise, "N/A");
    strcpy(sunset, "N/A");
    strcpy(midday, "N/A");

    // get location
    util_get_location(&latitude, &longitude, NULL);
    if (latitude == INVALID_NUMBER || longitude == INVALID_NUMBER) {
        printf("ERROR %s: failed to get gps location\n", progname);
        goto done;
    }

    // xxx
    sprintf(curl_url, "\"https://api.sunrise-sunset.org/json?lat=%0.4f&lng=%0.4f&formatted=0\"", latitude, longitude);
    ret = run_curl(curl_url, "curl.out");
    if (ret != 0) {
        printf("ERROR %s: run_curl failed\n", progname);
        goto done;
    }

    // xxx
    root = get_json_root("curl.out");
    if (root == NULL) {
        printf("ERROR %s: json parse failed\n", progname);
        goto done;
    }

    // get sunrise time
    rise = *util_json_get_value(root, "results", "sunrise", NULL);
    //printf("INFO %s: WEB  RISE %s\n", progname, rise.u.string);
    memset(&tm, 0, sizeof(tm));
    cnt = sscanf(rise.u.string, "%d-%d-%dT%d:%d:%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
    if (cnt != 6) {
        printf("ERROR %s: sscanf failed, cnt=%d\n", progname, cnt);
        goto done;
    }
    tm.tm_year -= 1900;
    tm.tm_mon -= 1;
    t_rise_gm = timegm(&tm);
    localtime_r(&t_rise_gm, &tm);
    sprintf(sunrise, "%02d:%02d", tm.tm_hour, tm.tm_min);

    // get sunset time
    set = *util_json_get_value(root, "results", "sunset", NULL);
    //printf("INFO %s: WEB  SET  %s\n", progname, set.u.string);
    memset(&tm, 0, sizeof(tm));
    cnt = sscanf(set.u.string, "%d-%d-%dT%d:%d:%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
    if (cnt != 6) {
        printf("ERROR %s: sscanf failed, cnt=%d\n", progname, cnt);
        goto done;
    }
    tm.tm_year -= 1900;
    tm.tm_mon -= 1;
    t_set_gm = timegm(&tm);
    localtime_r(&t_set_gm, &tm);
    sprintf(sunset, "%02d:%02d", tm.tm_hour, tm.tm_min);

    // get midday time
    t_midday_gm = (t_rise_gm + t_set_gm) / 2;
    localtime_r(&t_midday_gm, &tm);
    sprintf(midday, "%02d:%02d", tm.tm_hour, tm.tm_min);




    //make_local_time_str(rise.u.string, sunrise, "RISE");

    //set = *util_json_get_value(root, "results", "sunset", NULL);
    //make_local_time_str(set.u.string, sunset, "SET ");

done:
    util_json_free(root);
}

#if 0
static void make_local_time_str(char *str_in, char *str_out, char *debug)
{
    int cnt, hour, min, sec;
    char ampm[100];

    // extract hour,min,sec from str_in
    cnt = sscanf(str_in, "%d:%d:%d %s", &hour, &min, &sec, ampm);
    if (cnt != 4) {
        strcpy(str_out, "N/A");
        return;
    }

    // adjust pm time
    if (strcmp(ampm, "PM") == 0) {
        hour += 12;
    }

    // convert from utc to localtime
    hour -= 4;  // xxx todo

    // create str_out
    sprintf(str_out, "%02d:%02d", hour, min);
    //printf("INFO %s: WEB %s %s\n", progname, debug, str_out);
}
#endif

static int run_curl(char *url, char *filename)
{
    char cmd[500];
    int  ret;

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
        printf("ERROR %s: failed to read file %s/%s\n",
               progname, data_dir, filename);
        return NULL;
    }

    root = util_json_parse(str);
    if (root == NULL) {
        printf("ERROR %s: failed to parse json\n", progname);
        free(str);
        return NULL;
    }

    free(str);

    return root;
}
