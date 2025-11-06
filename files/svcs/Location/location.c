#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
//#include <stdbool.h>  // xxx check template too
//#include <unistd.h>

#include <sdlx.h>
#include <utils.h>

#include "svcs/Location/location.h"

// variables
char *progname;

// prototypes
int download_country_info(char *id);

// -----------------  MAIN  -----------------------------------------

int main(int argc, char **argv)
{
    int id, req, arg;
    bool done = false;

    // save args
    if (argc != 2) {
        printf("ERROR: args expected: id\n");
        return 1;
    }
    progname = argv[0];
    sscanf(argv[1], "%d", &id);

    // print starting msg
    printf("INFO %s: starting, id=%d\n", progname, id);

    // xxx test
    download_country_info("us");
    return 0;

    // service runtime loop
    while (!done) {
        // xxx svc processing
        printf("INFO %s service is running\n", progname);

        // wait for up to 3600 secs for a request
        svc_wait(id, 3600, &req, &arg);

        // if xxx svc stop is requested then break out of runtime loop
        switch (req) {
        case SVC_REQ_STOP:
            done = true;
            break;
        case SVC_LOCATION_REQ_GET_LOC_INFO:
            break;
        case SVC_LOCATION_REQ_COUNTRY_INFO_DOWNLOAD:
            break;
        case SVC_LOCATION_REQ_COUNTRY_INFO_DELETE:
            break;
        case SVC_LOCATION_REQ_COUNTRY_INFO_LIST:
            break;
        }
    }

    // print terminating msg
    printf("INFO %s: terminating\n", progname);
    return 0;
}

// -----------------  COUNTRY INFO DOWNLOAD  ------------------------

// xxx curl timeout option

int read_and_parse_json_file(char *filename);

int download_country_info(char *id)
{
    //char         cmd[100], dirname[100], *filename, *end_ptr;
    //void        *root = NULL;
    //char        *str = NULL, *str_orig;
    //int          ret, len, success_cnt=0, skip_cnt=0;
    //json_value_t name, latitude, longitude;
    int ret;
    char cmd[100], json_filename[100], fn[100];

    // download zip file containing city/town location and names
    // xxx allow replace
    sprintf(cmd, "curl -o %s/%s.zip https://www.geoapify.com/data-share/localities/%s.zip",
            progname, id, id);
    ret = system(cmd);
    if (ret != 0) {
        printf("ERROR %s: '%s' failed, ret=%d\n", progname, cmd, ret);
        return -1;
    }
    
    // unzip
    sprintf(cmd, "unzip -d %s %s/%s.zip", progname, progname, id);
    ret = system(cmd);
    if (ret != 0) {
        printf("ERROR %s: '%s' failed, ret=%d\n", progname, cmd, ret);
        return -1;
    }

    sprintf(json_filename, "%s/%s/%s", progname, id, "place_city.ndjson");
    ret = read_and_parse_json_file(json_filename);

    sprintf(json_filename, "%s/%s/%s", progname, id, "place-town.ndjson");
    ret = read_and_parse_json_file(json_filename);

    sprintf(json_filename, "%s/%s/%s", progname, id, "place-village.ndjson");
    ret = read_and_parse_json_file(json_filename);

    // xxx remove files
    sprintf(fn, "%s.zip", id);
    util_delete_file(progname, fn);
    util_delete_dir(progname, id);

    // xxx
    return ret;
}

int read_and_parse_json_file(char *json_filename)
{
    char         *end_ptr;
    void         *root = NULL;
    char         *str = NULL, *str_orig = NULL;
    int           len, success_cnt=0, skip_cnt=0;
    json_value_t  name, display_name, latitude, longitude;

    //printf("INFO %s: read_and_parse_json_file starting for %s\n", progname, json_filename);

    // read json into str_orig
    str_orig = util_read_file(".", json_filename, &len);
    if (str_orig == NULL) {
        printf("ERROR %s: failed read file %s\n", progname, json_filename);
        goto error;
    }

    // parse json
    str = str_orig;
    while (true) {
        // parse json
        root = util_json_parse(str, &end_ptr); //xxx get rid of const
        if (root == NULL) {
            printf("ERROR %s: util_json_parse failed, %s\n", progname, json_filename);
            goto error;
        }

        // extract json fields
        name         = *util_json_get_value(root, "name", NULL);
        display_name = *util_json_get_value(root, "display_name", NULL);
        latitude     = *util_json_get_value(root, "location", "0", NULL);
        longitude    = *util_json_get_value(root, "location", "1", NULL);

        // if fields extracted okay them save info, else skip
        if (name.type == JSON_TYPE_STRING &&
            display_name.type == JSON_TYPE_STRING &&
            latitude.type == JSON_TYPE_NUMBER &&
            longitude.type == JSON_TYPE_NUMBER)
        {
            printf("%f %f '%s' '%s'\n", latitude.u.number, longitude.u.number, name.u.string, display_name.u.string);
            success_cnt++;
        } else {
            //printf("ERROR %s: skipping - name,lat,long type = %d %d %d\n",
            //       progname, name.type, latitude.type, longitude.type); //xxx
            skip_cnt++;
        }

        // free the parsed json
        util_json_free(root);
        root = NULL;

        // advance str to the next json object
        str = end_ptr;
        while (*str != '{' && *str != '\0') {
            str++;
        }

        // if no more json objects then break
        if (*str == '\0') {
            break;
        }
    }

    // cleanup and return success
    printf("INFO %s: read_and_parse_json_file %s success_cnt=%d skip_cnt=%d\n", 
           progname, json_filename, success_cnt, skip_cnt);
    free(str_orig);
    util_json_free(root);
    return 0;

    // error return
error:
    free(str_orig);
    util_json_free(root);
    return -1;
}
