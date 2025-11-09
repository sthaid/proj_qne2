#ifndef __SVCS_H__
#define __SVCS_H__

#define SVC_REQ_STATE_NOT_ISSUED   0
#define SVC_REQ_STATE_PENDING      1
#define SVC_REQ_STATE_IN_PROGRESS  2
#define SVC_REQ_STATE_COMPLETED    3

typedef struct {
    int  req;
    int  state;
    char data[100];
} svc_req_t;

void svcs_init(void);
void svcs_stop_all(void);
void svcs_acquire_mutex(void);
void svcs_release_mutex(void);
void svcs_start_all_autostart_services(void);
void svcs_display(int bg_color);


#define SVC_REQ_NONE 0
#define SVC_REQ_STOP 1
void svc_call(char *svc_name, svc_req_t *req, bool wait);
void svc_wait(int id, int timeout_secs, int *req, int *arg);

#endif
