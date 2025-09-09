#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>

#include <sdl.h>

static char *app_dir;
static int   id;

int main(int argc, char **argv)
{
    int rc;
    double mag_heading;
    int cnt = 0;

    app_dir = argv[0];
    sscanf(argv[1], "%d", &id);

    printf("starting, app_dir = %s id = %d \n", app_dir, id);

    while (++cnt < 2000000) {
        if (stop_requested[id]) {
            printf("got stop request\n");
            break;
        }

        rc = sdl_sensor_read_mag_heading(&mag_heading);
        printf("t2: do some work, rc=%d mag_heading = %.0f\n", rc, mag_heading);

        sleep(1);
    }

    printf("terminating\n");
    stop_requested[5] = 0; // xxx need better way
    printf("terminating 2\n");
    return 0;
}
