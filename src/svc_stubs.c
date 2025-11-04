#include <std_hdrs.h>

#include <sdlx.h>

void svc_call(int id, int req, int arg)
{
}

void svc_wait(int id, int timeout_secs, int *req, int *arg)
{
    printf("svc_wait stub, sleeping %d secs ...\n", timeout_secs);
    sleep(timeout_secs);
}
