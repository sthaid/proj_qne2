#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include <sdlx.h>

int main(int argc, char **argv)
{
    char *progname, *data_dir;
    int   id;

    // save args
    if (argc != 3) {
        printf("ERROR: args expected: data_dir, id\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    sscanf(argv[2], "%d", &id);

    printf("INFO %s: starting, data_dir=%s id=%d\n", progname, data_dir, id);

    // runtime loop
    while (true) {
        // if request to stop received then break out of loop
        if (stop_requested[id]) {
            printf("INFO %s: got stop request\n", progname);
            break;
        }

        // print message
        printf("INFO %s service is running\n", progname);

        // sleep
        sleep(3);
    }

     printf("INFO %s: terminating\n", progname);
    return 0;
}
