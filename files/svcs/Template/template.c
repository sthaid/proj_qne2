#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

#include <sdlx.h>
#include <svcs.h>

// program args
char *progname;
char *data_dir;

// flag set when SVC_REQ_STOP received
bool end_program = false;

// prototypes
void process_req(svc_req_t *req);

// -----------------  TEMPLATE SERVICE  --------------------------------------

int main(int argc, char **argv)
{
    svc_req_t *req;
    long abstime;
    int rc;

    // save args
    if (argc != 2) {
        printf("ERROR: data_dir arg expected\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    // set absolute time at whcih svc_wiat_for_req will timeout;
    // this time is rounded down to the prior minute so that the first
    // call to svc_wait_for_req will timeout immedeately
    abstime = time(NULL) / 60 * 60;

    // service runtime loop
    while (!end_program) {
        // wait for req or timeout
        rc = svc_wait_for_req(progname, &req, abstime); //xxx use abstime

        // if scv_wait_for_req timedout then do periodic processing
        if (rc == SVC_WAIT_FOR_REQ_ERROR_TIMEDOUT) {
            printf("INFO %s: do some processing\n", progname);
            abstime += 60;
            continue;
        }

        // if svc_wait_for_req had some error other than the timeout handled above,
        // then short sleep and contune; perhaps the error will clear up
        if (rc != SVC_WAIT_FOR_REQ_SUCCESS) {
            sleep(10);
            continue;
        }

        // if req was recvd then process the req
        if (req != NULL) {
            printf("TEMPLETE  %p  %d  %p  %p  %p\n", req, req->req, 
                 &req->state, &req->comp_status, &req->req);
            process_req(req);
        }
    }
            
    // print terminating msg
    printf("INFO %s: terminating\n", progname);
    return 0;
}

void process_req(svc_req_t *req)
{
    printf("INFO %s: got req %d\n", progname, req->req);

    // process the request
    switch (req->req) {
    case SVC_REQ_STOP:
        svc_req_completed(req, SVC_REQ_COMP_STATUS_OK);
        end_program = true;
        break;
    default:
        printf("ERROR %s: req %d is invalid\n", progname, req->req);
        svc_req_completed(req, SVC_REQ_COMP_STATUS_ERROR_INVALID_REQ);
        break;
    }
}

