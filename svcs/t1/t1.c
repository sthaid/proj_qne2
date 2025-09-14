#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>

#include <sdl.h>

static char *progname;
static char *data_dir;
static int   id;

int main(int argc, char **argv)
{
    int    rc;
    double pressure;

    progname = argv[0];
    data_dir = argv[1];
    sscanf(argv[2], "%d", &id);

    printf("INFO %s: starting, data_dir = %s id = %d \n", progname, data_dir, id);

    rc = sdl_init(SUBSYS_SENSOR);
    if (rc != 0) {
        printf("ERROR %s: sdl_init failed\n", progname);
        return 1;
    }

    while (true) {
        if (stop_requested[id]) {
            printf("INFO %s: got stop request\n", progname);
            break;
        }

        rc = sdl_sensor_read_pressure(&pressure);
        printf("INFO %s: doing some work rc=%d pressure=%.0f\n", progname, rc, pressure);

        sleep(1);
    }

    sdl_exit();

    printf("INFO %s: terminating\n", progname);
    return 0;
}
