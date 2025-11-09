#include <stdio.h>
#include <stdbool.h>

#include <sdlx.h>
#include <svcs.h>

char *progname;  // argv[0]
char *data_dir;  // argv[1]

int main(int argc, char **argv)
{
    int id, req, arg;
    bool done = false;

    // save args
    if (argc != 2) {
        printf("ERROR: data_dir arg expected\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: data_dir=%s\n", progname, data_dir);

    // print starting msg
    printf("INFO %s: starting, id=%d\n", progname, id);

    // service runtime loop
    while (!done) {
        // print message
        printf("INFO %s service is running\n", progname);

        // wait for up to 3600 secs for a request;
        // if no req received within timeout then continue
        svc_wait_for_req(progname, 3600, &req); //xxx use abstime
        if (req == NULL) {
            continue;
        }

        // process the request
        switch (req->req) {
        case SVC_REQ_STOP:
            done = true;
            svc_set_req_complete(req, SVC_REQ_COMP_OK);
            break;
        default
            svc_set_req_complete(req, SVC_REQ_COMP_ERROR_INVALID_REQ);
            break;
        }
    }

    // print terminating msg
    printf("INFO %s: terminating\n", progname);
    return 0;
}
