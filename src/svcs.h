#ifndef __SVCS_H__
#define __SVCS_H__

// xxx comments needed

void svcs_init(void);
void svcs_stop_all(void);
void svcs_acquire_mutex(void);
void svcs_release_mutex(void);
void svcs_start_all_autostart_services(void);
void svcs_display(int bg_color);

// ----------------------------

// common values for svc_req_t req
#define SVC_REQ_STOP 1

// values returned by svc_issue_req
#define SVC_ISSUE_REQ_SUCCESS              0
#define SVC_ISSUE_REQ_ERROR_SVC_NOT_FOUND  1
#define SVC_ISSUE_REQ_ERROR_QUEUE_FULL     2

// values returned by svc_wait_for_req
#define SVC_WAIT_FOR_REQ_SUCCESS               0
#define SVC_WAIT_FOR_REQ_ERROR_SVC_NOT_FOUND   1
#define SVC_WAIT_FOR_REQ_ERROR_TIMEDOUT        2
#define SVC_WAIT_FOR_REQ_ERROR_OTHER           3

// values for svc_req_t status
#define SVC_REQ_STATUS_NOT_COMPLETE         0
#define SVC_REQ_STATUS_OK                   1
#define SVC_REQ_STATUS_ERROR_QUEUE_FULL     2
#define SVC_REQ_STATUS_ERROR_INVALID_REQ    3
#define SVC_REQ_STATUS_ERROR_SVC_NOT_FOUND  4
#define SVC_REQ_STATUS_ERROR_OTHER          5

// sizeof of req->data
#define MAX_SVC_REQ_DATA 100

typedef struct {
    int  req;
    int  status;
    char data[MAX_SVC_REQ_DATA];
} svc_req_t;

// routines called by apps
int svc_issue_req(char *svc_name, svc_req_t *req);
bool svc_is_req_complete(svc_req_t *req);
void svc_wait_for_req_complete(svc_req_t *req, int timeout_secs);

// routines called by svcs
int svc_wait_for_req(char *svc_name, svc_req_t **req, long timeout_abstime_secs);
void svc_req_completed(svc_req_t *req, int comp_status);

#endif
