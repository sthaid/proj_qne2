#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <sdlx.h>
#include <utils.h>

// variables
char *progname;
char *data_dir;
    
// prototypes
//xxx int download_city_and_town_locations(char *id);

// -----------------  MAIN  ------------------------------------------
    
int main(int argc, char **argv)
{
    int          rc;
    sdlx_event_t event;
    bool         done = false;

    // save args
    if (argc != 2) {
        printf("ERROR: data_dir arg expected\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    // xxx
    //download_city_and_town_locations("us");
    //return 1;

    // init sdl video subsystem
    rc = sdlx_init(SUBSYS_VIDEO);
    if (rc != 0) {
        printf("ERROR %s: sdlx_init failed\n", progname);
        return 1;
    }

    // runtime loop
    while (!done) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // register control event to
        // - end program
        sdlx_register_control_events(NULL, NULL, "X", COLOR_BLACK, 0, 0, EVID_QUIT);

        // xxx
        // display 'Location' at center of display
        sdlx_render_printf_xyctr(sdlx_win_width/2, sdlx_win_height/2, "Location");

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

    // cleanup and end program
    sdlx_quit(SUBSYS_VIDEO);
    printf("INFO %s: terminating\n", progname);
    return 0;
}

#if 0  // xxx in the svc
// -----------------  DOWNLOAD CITY & TOWN LOCATIONS  ----------------

int read_and_parse_json_file(char *filename);

int download_city_and_town_locations(char *id)
{
    //char         cmd[100], dirname[100], *filename, *end_ptr;
    //void        *root = NULL;
    //char        *str = NULL, *str_orig;
    //int          ret, len, success_cnt=0, skip_cnt=0;
    //json_value_t name, latitude, longitude;
    int ret;
    char cmd[100], filename[100];

    goto skip;

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
    sprintf(cmd, "unzip %s/%s.zip", progname, id);
    ret = system(cmd);
    if (ret != 0) {
        printf("ERROR %s: '%s' failed, ret=%d\n", progname, cmd, ret);
        return -1;
    }

skip:
    sprintf(filename, "%s/%s/%s", progname, id, "place_city.ndjson");
    ret = read_and_parse_json_file(filename);

    sprintf(filename, "%s/%s/%s", progname, id, "place-town.ndjson");
    ret = read_and_parse_json_file(filename);

    sprintf(filename, "%s/%s/%s", progname, id, "place-village.ndjson");
    ret = read_and_parse_json_file(filename);

    // xxx remove files

    return ret;
}

int read_and_parse_json_file(char *filename)
{
    char         *end_ptr;
    void         *root = NULL;
    char         *str = NULL, *str_orig = NULL;
    int           len, success_cnt=0, skip_cnt=0;
    json_value_t  name, display_name, latitude, longitude;

    printf("INFO %s: read_and_parse_json_file starting for %s\n", progname, filename);

    // xxx
    str_orig = util_read_file(".", filename, &len);
    if (str_orig == NULL) {
        printf("ERROR %s: failed read file %s\n", progname, filename);
        goto error;
    }

    // parse json
    str = str_orig;
    while (true) {
        root = util_json_parse(str, &end_ptr); //xxx get rid of const
        if (root == NULL) {
            printf("ERROR %s: util_json_parse failed\n", progname);
            goto error;
        }

        name         = *util_json_get_value(root, "name", NULL);
        display_name = *util_json_get_value(root, "display_name", NULL);
        latitude     = *util_json_get_value(root, "location", "0", NULL);
        longitude    = *util_json_get_value(root, "location", "1", NULL);

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

        util_json_free(root);
        root = NULL;

        str = end_ptr;
        while (*str != '{' && *str != '\0') {
            str++;
        }
        if (*str == '\0') {
            break;
        }
    }

    printf("INFO %s: read_and_parse_json_file successes=%d skips=%d\n", 
           progname, success_cnt, skip_cnt);

    // free allocated str, and return succss
    free(str_orig);
    util_json_free(root);
    return 0;

    // error return
error:
    free(str_orig);
    util_json_free(root);
    return -1;
}
#endif
