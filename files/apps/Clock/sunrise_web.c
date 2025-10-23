#include <stdio.h>  // xxx
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
    
#include <sdlx.h> //xxx
#include <utils.h>

#include "apps/Clock/common.h"

void sunrise_sunset_web(char *sunrise, char *sunset, char *midday)
{
}

#if 0

void test2(void)
{
    char         curl_url[200];
    json_value_t rise, set;
    int          ret;
    double       latitude, longitude;
    void        *root;

    // get location
    util_get_location(&latitude, &longitude, NULL);
    if (latitude == INVALID_NUMBER || longitude == INVALID_NUMBER) {
        printf("ERROR %s: failed to get gps location\n", progname);
        goto done;
    }

    // xxx
    sprintf(curl_url, "\"https://api.sunrise-sunset.org/json?lat=%0.4f&lng=%0.4f\"", latitude, longitude);
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

    rise = *util_json_get_value(root, "results", "sunrise", NULL);
    set = *util_json_get_value(root, "results", "sunset", NULL);
    printf("RISE %s\n", rise.u.string);
    printf("SET  %s\n", set.u.string);

done:
    util_json_free(root);
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
#endif
