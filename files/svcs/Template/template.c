#include <stdio.h>
#include <stdbool.h>

#include <sdlx.h>

char *progname;

int main(int argc, char **argv)
{
    int id, req, arg;

    // save args
    if (argc != 2) {
        printf("ERROR: args expected: id\n");
        return 1;
    }
    progname = argv[0];
    sscanf(argv[1], "%d", &id);

    // print starting msg
    printf("INFO %s: starting, id=%d\n", progname, id);

    // service runtime loop
    while (true) {
        // print message
        printf("INFO %s service is running\n", progname);

        // wait for up to 3600 secs for a request
        svc_wait(id, 3600, &req, &arg);

        // if svc stop is requested then break out of runtime loop
        if (req == SVC_REQ_STOP) {
            break;
        }
    }

    // print terminating msg
    printf("INFO %s: terminating\n", progname);
    return 0;
}
