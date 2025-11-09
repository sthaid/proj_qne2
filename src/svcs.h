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

// values for svc_req_t req
#define SVC_REQ_STOP 1

// values for svc_req_t state
#define SVC_REQ_STATE_NOT_ISSUED        0
#define SVC_REQ_STATE_QUEUED            1
#define SVC_REQ_STATE_IN_PROGRESS       2
#define SVC_REQ_STATE_COMPLETE          3

// values for svc_req_t comp_status
#define SVC_REQ_COMP_STATUS_NOT_COMPLETE       0
#define SVC_REQ_COMP_STATUS_OK                 1
#define SVC_REQ_COMP_STATUS_ERROR              2
#define SVC_REQ_COMP_STATUS_ERROR_QUEUE_FULL   3
#define SVC_REQ_COMP_STATUS_ERROR_INVALID_REQ  4
#define SVC_REQ_COMP_STATUS_ERROR_INVLD_SVC_NAME  5

typedef struct {
    int  req;
    int  state;
    int  comp_status;
    char data[100];
} svc_req_t;

// routines called by apps
void svc_issue_req(char *svc_name, svc_req_t *req);
bool svc_is_req_complete(svc_req_t *req);
void svc_wait_for_req_complete(svc_req_t *req, int timeout_secs);

// routines called by svcs
void svc_wait_for_req(char *svc_name, svc_req_t **req, int timeout_abstime_secs);
void svc_req_completed(svc_req_t *req, int comp_status);

#endif
