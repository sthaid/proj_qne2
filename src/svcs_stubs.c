#include <std_hdrs.h>

#include <svcs.h>
#include <logging.h>

//
// routines called by apps
//

void svc_issue_req(char *svc_name, svc_req_t *req)
{
    INFO("STUB svc_name = %s req = %d\n", svc_name, req->req);
}

bool svc_is_req_complete(svc_req_t *req)
{
    return false;
}

void svc_wait_for_req_complete(svc_req_t *req, int timeout_secs)
{
}

//
// routines called by svcs
//

void svc_wait_for_req(char *svc_name, svc_req_t **req, int timeout_abstime_secs)
{
    INFO("STUB svc_name = %s timeout_abstime_secs = %d\n", svc_name, timeout_abstime_secs);
// xxxx sleep until the abstime
    sleep(300);
    *req = NULL;
}

void svc_req_completed(svc_req_t *req, int comp_status)
{
}

