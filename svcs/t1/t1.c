#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>

#include <sdl.h>

#ifdef __GNUC__
static char stop_requested[100];
#endif

static char *progname;
static char *svc_dir;
static int   id;

int main(int argc, char **argv)
{
    int    rc, cnt = 0;
    double mag_heading;

    // xxx does sdl_init need to be called for service

    progname = argv[0];
    svc_dir = argv[1];
    sscanf(argv[2], "%d", &id);

    printf("INFO %s: starting, svc_dir = %s id = %d \n", progname, svc_dir, id);

    while (++cnt < 2000000) {
        if (stop_requested[id]) {
            printf("INFO %s: got stop request\n", progname);
            break;
        }

        rc = sdl_sensor_read_mag_heading(&mag_heading);
        printf("INFO %s: do some work, rc=%d mag_heading = %.0f\n", progname, rc, mag_heading);

        sleep(1);
    }

    printf("INFO %s: terminating\n", progname);
    return 0;
}
