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
    ERROR("STUB svc_issue_req: svc_name %s not found\n", svc_name);
    svc_req_completed(req, SVC_REQ_STATUS_ERROR_SVC_NOT_FOUND);
    return SVC_ISSUE_REQ_ERROR_SVC_NOT_FOUND;
}

bool svc_is_req_complete(svc_req_t *req)
{
    return true;
}

void svc_wait_for_req_complete(svc_req_t *req, int timeout_secs)
{
    sleep(timeout_secs);
}

char *svc_make_req(char *svc_name, int req_id, char *data_in, int timeout_secs)
{
    ERROR("STUB svc_make_req: svc_name %s not found\n", svc_name);
    sleep(timeout_secs);
    return NULL;
}

//
// routines called by svcs
//

int svc_wait_for_req(char *svc_name, svc_req_t **req, long timeout_abstime_secs)
{
    ERROR("STUB svc_wait_for_req: svc_name %s not found\n", svc_name);
    *req = NULL;
    return SVC_WAIT_FOR_REQ_ERROR_SVC_NOT_FOUND;
}

void svc_req_completed(svc_req_t *req, int status)
{
    __sync_synchronize();
    req->status = status;
}

