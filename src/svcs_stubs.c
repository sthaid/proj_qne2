#include <std_hdrs.h>

#include <svcs.h>
#include <logging.h>
#include <utils.h>

#define MS  1000L
#define SEC 1000000L

//
// routines called by apps
//

int svc_issue_req(char *svc_name, svc_req_t *req)
{
    ERROR("STUB svc_name %s not found\n", svc_name);
    svc_req_completed(req, SVC_REQ_COMP_STATUS_ERROR_SVC_NOT_FOUND);
    return SVC_ISSUE_REQ_ERROR_SVC_NOT_FOUND;
}

bool svc_is_req_complete(svc_req_t *req)
{
    return true;
}

void svc_wait_for_req_complete(svc_req_t *req, int timeout_secs)
{
    long start_us;

    start_us = util_microsec_timer();
    while (true) {
        if (req->comp_status != SVC_REQ_COMP_STATUS_NOT_COMPLETE) {
            break;
        }
        if (util_microsec_timer() - start_us > timeout_secs * SEC) {
            break;
        }
        usleep(100*MS);
    }
}

//
// routines called by svcs
//

int svc_wait_for_req(char *svc_name, svc_req_t **req, long timeout_abstime_secs)
{
    ERROR("STUB svc_name %s not found\n", svc_name);
    *req = NULL;
    return SVC_WAIT_FOR_REQ_ERROR_SVC_NOT_FOUND;
}

void svc_req_completed(svc_req_t *req, int comp_status)
{
    __sync_synchronize();
    req->state = SVC_REQ_STATE_COMPLETE;
    req->comp_status = comp_status;
}

