#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include <sdl.h>
#include <utils.h>

#define DATA_MAGIC 0x55aa66bb
typedef struct {
    int initialized;
    int max;
    int next;
    int pad;
    struct {
        int tbd;
    } values[100000];
} data_t;

static char *progname;
static char *data_dir;
static int   id;

int main(int argc, char **argv)
{
    data_t *data = NULL;
    int     rc;

    // save args
    progname = argv[0];
    data_dir = argv[1];
    sscanf(argv[2], "%d", &id);

    // print starting message
    printf("INFO %s: starting, data_dir = %s id = %d \n", progname, data_dir, id);

    // init the SDL sensor subsystem
    rc = sdl_init(SUBSYS_SENSOR);
    if (rc != 0) {
        printf("ERROR %s: failed to init SUBSYS_SENSOR\n", progname);
        return 1;
    }

    // map the sensors.dat file, the file will be created if it doesnt exist
    data = util_map_file(data_dir, "sensors.dat", sizeof(data_t), true);
    if (data == NULL) {
        printf("ERROR %s: failed to map sensors.dat\n", progname);
        return 1;
    }

    // if sensors.dat was created then initialize it
    if (data->initialized != DATA_MAGIC) {
        data->initialized = DATA_MAGIC;
        // xxx and other fields
        util_sync_file(&data->initialized, sizeof(int));
    }

    // loop
    while (true) {
        // if request to stop received then break out of loop
        if (stop_requested[id]) {
            printf("INFO %s: got stop request\n", progname);
            break;
        }  
    
        // sleep until end of hour xxx needs to loop for stop_req
        sleep(1);

        // read sensors

        // add entry to sensors.dat
    }

    // cleanup
    util_unmap_file(data);
    sdl_quit(SUBSYS_SENSOR);

    // end program
    printf("INFO %s: terminating\n", progname);
    return 0;
}

