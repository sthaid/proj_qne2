#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
//#include <stdbool.h>  // xxx check template too
//#include <unistd.h>

#include <sdlx.h>
#include <utils.h>

#include "svcs/Location/location.h"

// defines
#define BOLTON_MASS_LATITUDE     42.4334
#define BOLTON_MASS_LONGITUDE   -71.6078

// typedefs
#define MAX_NAME 32
typedef struct {
    double latitude;
    double longitude;
    char   name[MAX_NAME];
} loc_t;

// variables
char   *progname;
loc_t  *loc;
int     max_loc;

// prototypes
void read_location_data(void);
void find_closest(double latitude, double longitude);
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

    // read location data
    read_location_data();

    // service runtime loop
    while (!done) {
        // service processing
        find_closest(BOLTON_MASS_LATITUDE, BOLTON_MASS_LONGITUDE);

        // wait for up to 3600 secs for a request
        svc_wait(id, 3600, &req, &arg);

        // if xxx svc stop is requested then break out of runtime loop
        switch (req) {
        case SVC_REQ_STOP:
            done = true;
            break;
        case SVC_LOCATION_REQ_GET_LOC_INFO:
            find_closest(BOLTON_MASS_LATITUDE, BOLTON_MASS_LONGITUDE);
            break;
        case SVC_LOCATION_REQ_COUNTRY_INFO_DOWNLOAD:
            download_country_info("us");
            break;
        case SVC_LOCATION_REQ_COUNTRY_INFO_DELETE:
            break;
        case SVC_LOCATION_REQ_COUNTRY_INFO_LIST:
            break;
        }
    }

    // cleanup and end program
    free(loc);
    printf("INFO %s: terminating\n", progname);
    return 0;
}

void read_location_data(void)
{
    char cmd[200];
    int  len;

    // init number of loc to 0
    max_loc = 0;

    // catenate the *.loc files
    sprintf(cmd, "cat %s/*.loc > %s/all_loc", progname, progname);
    system(cmd);

    // read the catenated file
    loc = util_read_file(progname, "all_loc", &len);
    if (loc == NULL) {
        printf("ERROR %s: failed to read all_loc\n", progname);
        return;
    }

    // xxx maybe delete all_loc when done with it

    // set max_loc
    if ((len % sizeof(loc_t)) != 0) {
        printf("ERROR %s: invalid len %d of file all_loc\n", progname, len);
        return;        
    }
    max_loc = len / sizeof(loc_t);
    printf("INFO %s: max_loc = %d\n", progname, max_loc);
}

// -----------------  FIND CLOSEST LOCAATION  -----------------------

// There are approximately 364,000 feet (\(69\) miles) in one degree of latitude
//
// The number of feet in one degree of longitude varies based on your latitude,
// decreasing from approximately 364,000 feet (69 miles) at the equator to zero
// at the poles. For a specific location, you can calculate this distance by
// multiplying the distance at the equator by the cosine of your latitude

// xxx what if not found
void find_closest(double latitude, double longitude)
{
    double delta_lat, delta_long, cos_lat;
    double ns_feet, ew_feet, distance_squared, min_distance_squared, min_distance;
    double point5_div_cos_lat;
    char min_name[MAX_NAME];

    min_distance_squared = 1e99;
    cos_lat = cos(latitude * (M_PI / 180));
    point5_div_cos_lat = 0.5 / cos_lat;

    //printf("min_dist start %f  cos_lat %f\n", min_distance_squared, cos_lat);
    //printf("point5_div_cos_lat = %f\n", point5_div_cos_lat);

    for (int i = 0; i < max_loc; i++) {
        loc_t *x = &loc[i];

        delta_lat = fabs(latitude - x->latitude);
        //printf("delta_lat %f    %f %f\n", delta_lat, latitude, x->latitude);
        if (delta_lat > 0.5) {
            continue;
        }
        delta_long = fabs(longitude - x->longitude);  // xxx deal with longitude near +/-180
        if (delta_long > point5_div_cos_lat) {
            continue;
        }

        printf("IN BOX %s\n", x->name);

        ns_feet = delta_lat;  // xxx optimize
        ew_feet = delta_long * cos_lat;
        //printf("xxxxxxx %f %f\n", ns_feet, ew_feet);
        distance_squared = (ns_feet * ns_feet) + (ew_feet * ew_feet);

        if (distance_squared < min_distance_squared) {
            //printf("min_name = %s,  lat/long=%f %f\n", x->name, x->latitude, x->longitude);
            strncpy(min_name, x->name, MAX_NAME);
            min_name[MAX_NAME-1] = '\0';
            min_distance_squared = distance_squared;
        }
    }

    min_distance = 364000 * sqrt(min_distance_squared) / 5280;
    printf("INFO %s: name = %s  distance = %f\n", progname, min_name, min_distance);
}

// -----------------  COUNTRY INFO DOWNLOAD  ------------------------

int read_and_parse_json_file(char *json_filename, FILE *fp_out);

int download_country_info(char *id)
{
    int ret = -1;
    char cmd[100], json_filename[100], out_filename[100], zip_filename[100];
    FILE *fp_out = NULL;

    // init
    sprintf(zip_filename, "%s.zip", id);

    // create output file
    sprintf(out_filename, "%s/%s.loc", progname, id);
    fp_out = fopen(out_filename, "wb");
    if (fp_out == NULL) {
        printf("ERROR %s: failed to create %s\n", progname, out_filename);
        goto done;
    }

    // download zip file containing city/town location and names
    if (0) {
    util_delete_file(progname, zip_filename);
    sprintf(cmd, "curl --silent --max-time 10 --output %s/%s.zip https://www.geoapify.com/data-share/localities/%s.zip",
            progname, id,  id);
    ret = system(cmd);
    if (ret != 0) {
        printf("ERROR %s: '%s' failed, ret=%d\n", progname, cmd, ret);
        goto done;
    }
    }
    
    // unzip
    sprintf(cmd, "unzip -o -d %s %s/%s.zip", progname, progname, id);
    ret = system(cmd);
    if (ret != 0) {
        printf("ERROR %s: '%s' failed, ret=%d\n", progname, cmd, ret);
        goto done;
    }

    // parse the json files to obtain city/town/village names and latitude/longitude;
    // these will be written to the file associated with 'fp_out'
    sprintf(json_filename, "%s/%s/%s", progname, id, "place_city.ndjson");
    read_and_parse_json_file(json_filename, fp_out);

    sprintf(json_filename, "%s/%s/%s", progname, id, "place-town.ndjson");
    read_and_parse_json_file(json_filename, fp_out);

    sprintf(json_filename, "%s/%s/%s", progname, id, "place-village.ndjson");
    read_and_parse_json_file(json_filename, fp_out);

    // set ret to success
    ret = 0;

done:
    // cleanup
    //util_delete_file(progname, zip_filename);  xxx
    //util_delete_dir(progname, id);  xxx
    if (fp_out) {
        fclose(fp_out);
    }

    // return status
    return ret;
}

int read_and_parse_json_file(char *json_filename, FILE *fp_out)
{
    char         *end_ptr;
    void         *root = NULL;
    char         *str = NULL, *str_orig = NULL;
    int           len, success_cnt=0, skip_cnt=0;
    json_value_t  name, latitude, longitude;

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
        longitude    = *util_json_get_value(root, "location", "0", NULL);
        latitude     = *util_json_get_value(root, "location", "1", NULL);

        // if fields extracted okay them save info, else skip
        if (name.type == JSON_TYPE_STRING &&
            latitude.type == JSON_TYPE_NUMBER &&
            longitude.type == JSON_TYPE_NUMBER)
        {
            loc_t x;
            x.latitude = latitude.u.number;
            x.longitude = longitude.u.number;
            strncpy(x.name, name.u.string, MAX_NAME);
            x.name[MAX_NAME-1] = '\0';

            fwrite(&x, sizeof(loc_t), 1, fp_out);
            success_cnt++;
        } else {
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
