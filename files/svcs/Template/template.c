#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include <sdlx.h>
#include <svcs.h>

char *progname;  // argv[0]
char *data_dir;  // argv[1]

int main(int argc, char **argv)
{
    bool done = false;
    svc_req_t *req;
    time_t tnow;

    // save args
    if (argc != 2) {
        printf("ERROR: data_dir arg expected\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    // service runtime loop
    while (!done) {
        // print message
        printf("INFO %s: service is running\n", progname);

        // wait for up to 3600 secs for a request;
        // if no req received within timeout then continue
        tnow = time(NULL);
        svc_wait_for_req(progname, &req, tnow+3600); //xxx use abstime
        if (req == NULL) {
            printf("INFO %s: no req\n", progname);
            continue;
        }

        // process the request
        switch (req->req) {
        case SVC_REQ_STOP:
            done = true;
            svc_req_completed(req, SVC_REQ_COMP_STATUS_OK);
            break;
        default:
            svc_req_completed(req, SVC_REQ_COMP_STATUS_ERROR_INVALID_REQ);
            break;
        }
    }

    // print terminating msg
    printf("INFO %s: terminating\n", progname);
    return 0;
}
