#include <std_hdrs.h>

#include <sdlx.h>
#include <utils.h>
#include <logging.h>
#include <svcs.h>
#include <main.h>

//
// defines
//

#define SERVICE_STATE_STOPPED           0     // white
#define SERVICE_STATE_RUNNING           1     // green
#define SERVICE_STATE_STOPPING          2     // yellow
#define SERVICE_STATE_STOPPED_BY_ERROR  3     // red

#define SERVICE_STATE_STR(state) \
    ((state) == SERVICE_STATE_STOPPED           ? "STOPPED"          : \
     (state) == SERVICE_STATE_RUNNING           ? "RUNNING"          : \
     (state) == SERVICE_STATE_STOPPING          ? "STOPPING"         : \
     (state) == SERVICE_STATE_STOPPED_BY_ERROR  ? "STOPPED_BY_ERROR" : \
                                                  "????")

#define SERVICE_STATE_TO_COLOR(state) \
    ((state) == SERVICE_STATE_STOPPED           ? COLOR_WHITE  : \
     (state) == SERVICE_STATE_RUNNING           ? COLOR_GREEN  : \
     (state) == SERVICE_STATE_STOPPING          ? COLOR_YELLOW : \
                                                  COLOR_RED)

#define SERVICE_IS_STOPPED(state)  ((state) == SERVICE_STATE_STOPPED || \
                                    (state) == SERVICE_STATE_STOPPED_BY_ERROR)

#define MAX_SVCS 20

#define MS  1000

//
// typedefs
//

typedef struct {
    char            name[30];
    char            autostart;   // y or n
    int             state;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    svc_req_t      *req;
} svc_t;

//
// variables
//

static svc_t  svcs[MAX_SVCS];
static int    max_svcs;

//
// prototypes
//

static void process_svc_start_req(int id);
static void process_svc_stop_req(int id);
static void process_svc_stopped_callback(int id, int rc);
static void run_svc(int id);

// -----------------  SVCS ROUTINES USED BY MAIN.C  ---------------

void svcs_init(void)
{
    static bool first_call = true;
    FILE *fp;
    char str[100];
    int cnt, line=0;

    // on first call init the pthread mutex and condition
    if (first_call) {
        for (int i = 0; i < MAX_SVCS; i++) {
            pthread_mutex_init(&svcs[i].mutex, NULL);
            pthread_cond_init(&svcs[i].cond, NULL);
        }
        first_call = false;
    }

    // stop all svcs
    // xxx what if this doesnt stop them all?
    svcs_stop_all();

    // get svc names and autostart indicator from the svcs file
    max_svcs = 0;
    fp = fopen("svcs/svcs", "r");
    if (fp == NULL) {
        ERROR("failed to open svcs/svcs, %s\n", strerror(errno));
        return;
    }
    while (fgets(str, sizeof(str), fp) != NULL) {
        svc_t *x = &svcs[max_svcs];
        line++;

        // ignore lines that begin with '#', space, or newline
        if (str[0] == '\n' || str[0] == ' ' || str[0] == '#') {
            continue;
        }

        // line format: <name> <y/n>
        cnt = sscanf(str, "%s %c", x->name, &x->autostart);
        if (cnt != 2) {
            ERROR("invalid line %d in svcs file\n", line);
            fclose(fp);
            max_svcs = 0;
            return;
        }

        // increment max_svcs
        max_svcs++;

        // if svcs table is full print warning and break
        if (max_svcs == MAX_SVCS) {
            WARN("svcs tbl is full\n");
            break;
        }
    }
    fclose(fp);

    // start all autostart svcs
    for (int id = 0; id < max_svcs; id++) {
        svc_t *x = &svcs[id];
        if (SERVICE_IS_STOPPED(x->state) && x->autostart == 'y') {
            x->req = NULL;
            x->state = SERVICE_STATE_RUNNING;
            run_svc(id);
        }
    }
}

void svcs_display(int bg_color)
{
    sdlx_event_t event;
    int         id;
    bool        done = false;
    sdlx_loc_t  *loc;
    double      row;

    // xxx use max_svcs in these defines
    #define EVID_SVC_START    100
    #define EVID_SVC_STOP     200

    // handle the setting display
    while (true) {
        // init display and display title line
        sdlx_display_init(bg_color);
        sdlx_print_init(DEFAULT_FONT, COLOR_WHITE, bg_color);
        sdlx_render_text_xyctr(sdlx_win_width/2, sdlx_char_height/2, "Services");

        // display name and controls for each service
        row = 2;
        for (id = 0; id < max_svcs; id++) {
            svc_t *x = &svcs[id];

            sdlx_print_init_color(SERVICE_STATE_TO_COLOR(x->state), bg_color);
            sdlx_render_printf(0, ROW2Y(row), "%-s", x->name);

            sdlx_print_init_color(COLOR_LIGHT_BLUE, bg_color);
            if (SERVICE_IS_STOPPED(x->state)) {
                loc = sdlx_render_printf(COL2X(10), ROW2Y(row), "start");
                sdlx_register_event(loc, EVID_SVC_START+id);
            } else if (x->state == SERVICE_STATE_RUNNING) {
                loc = sdlx_render_printf(COL2X(10), ROW2Y(row), "stop");
                sdlx_register_event(loc, EVID_SVC_STOP+id);
            }

            row += 1.5;
        }

        // display the control event 'X' to exit this
        sdlx_register_control_events(NULL, NULL, "X", bg_color, 0, 0, EVID_QUIT);

        // present the display
        sdlx_display_present();

        // wait for an event, with 100 ms timeout;
        // if no event received then re-display
        sdlx_get_event(100*MS, &event);
        if (event.event_id == -1) {
            continue;
        }

        // process the event
        INFO("proc event_id %d\n", event.event_id);
        switch (event.event_id) {
        case EVID_SVC_START ... EVID_SVC_START + MAX_SVCS - 1:
            id = event.event_id - EVID_SVC_START;
            process_svc_start_req(id);
            break;
        case EVID_SVC_STOP ... EVID_SVC_STOP + MAX_SVCS - 1:
            id = event.event_id - EVID_SVC_STOP;
            process_svc_stop_req(id);
            break;
        case EVID_QUIT:
            done = true;
            break;
        }

        if (done) {
            break;
        }
    }
}

void svcs_stop_all(void)
{
    int id, duration_ms = 0;
    bool all_stopped;
    svc_req_t *req;

    INFO("stopping all services\n");

xxx call the process_xxx
    for (id = 0; id < max_svcs; id++) {
        svc_t *x = &svcs[id];
        if (x->state == SERVICE_STATE_RUNNING) {
            req = calloc(1, sizeof(svc_req_t));
            req->req = SVC_REQ_STOP;
            svc_call(svcs[id].name, req, true);
            free(req);
        }
    }

    while (true) {
        all_stopped = true;
        for (id = 0; id < max_svcs; id++) {
            svc_t *x = &svcs[id];
            if (SERVICE_IS_STOPPED(x->state)) {
                all_stopped = false;
                break;
            }
        }

        if (all_stopped) {
            INFO("all services are stopped\n");
            break;
        }

        if (duration_ms > 30000) {
            ERROR("the following services have failed to stop ...\n");
            for (id = 0; id < max_svcs; id++) {
                svc_t *x = &svcs[id];
                if (!SERVICE_IS_STOPPED(x->state)) {
                    ERROR("- %-12s %s\n", x->name, SERVICE_STATE_STR(x->state));
                }
            }
            break;
        }

        usleep(100*MS);
        duration_ms += 100;
    }
}

// -----------------  SVCS ROUTINES AVAIL IN PICOC  ---------------

void svc_call(char *svc_name, svc_req_t *req, bool wait)
{
    int id;

    // find caller requested svc_name in the svcs table
    for (id = 0; id < max_svcs; id++) {
        if (strcmp(svcs[id].name, svc_name) == 0) {
            break;
        }
    }
    if (id == max_svcs) {
        ERROR("svc_name %s not found\n", svc_name);
        return;
    }

    INFO("id=%d name=%s req=%d\n", id, svcs[id].name, req->req);

    // acquire mutex
    pthread_mutex_lock(&svcs[id].mutex);

    // signal the svc so it will perform the request
    req->state = SVC_REQ_STATE_PENDING;
    svcs[id].req = req;
    pthread_cond_signal(&svcs[id].cond);

    // release mutex
    pthread_mutex_unlock(&svcs[id].mutex);

    // if caller requested wait for completion then poll for req competion
    if (wait) {
        INFO("waiting for req %d %s to complete\n", req->req, svc_name);
        while (true) {
            if (req->state == SVC_REQ_STATE_COMPLETED) {
                break;
            }
            usleep(100*MS);
        }
        INFO("completed req %d %s\n", req->req, svc_name);
    }
}

void svc_wait(int id, int timeout_secs, int *req, int *arg)
{
#if 0
    struct timespec ts;
    int             ret;

    INFO("called id=%d timeout_secs=%d\n", id, timeout_secs);

    // acquire mutex
    pthread_mutex_lock(&svc_request[id].mutex);

    // wait for xxx
    while (svc_request[id].req == SVC_REQ_NONE) {
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout_secs;
        ret = pthread_cond_timedwait(&svc_request[id].cond, &svc_request[id].mutex, &ts);
        INFO("pthread_cond_timedwait, ret=%d\n", ret);
        if (ret == ETIMEDOUT) {
            INFO("got ETIMEDOUT\n");
            break;
        } 
    }

    // xxxx
    *req = svc_request[id].req;
    *arg = svc_request[id].arg;
    svc_request[id].req = SVC_REQ_NONE;
    svc_request[id].arg = 0;
    INFO("return req=%d arg=%d\n", *req, *arg);

    // release mutex
    pthread_mutex_unlock(&svc_request[id].mutex);
#endif
}

// -----------------  HANDLERS  -----------------------------------

static void process_svc_start_req(int id)
{
    svc_t *x = &svcs[id];

    INFO("called for id=%d name=%s\n", id, x->name);

    if (!SERVICE_IS_STOPPED(x->state)) {
        ERROR("id=%d name=%s state=%s\n", id, x->name, SERVICE_STATE_STR(x->state));
        return;
    }
    
    x->state = SERVICE_STATE_RUNNING;
    x->req = NULL;
    run_svc(id);
}

static void process_svc_stop_req(int id)
{
    svc_t *x = &svcs[id];
    svc_req_t *req;

    INFO("called for id=%d name=%s\n", id, x->name);

    if (x->state != SERVICE_STATE_RUNNING) {
        ERROR("id=%d name=%s state=%s\n", id, x->name, SERVICE_STATE_STR(x->state));
        return;
    }

    x->state = SERVICE_STATE_STOPPING;

    req = calloc(1, sizeof(svc_req_t));
    req->req = SVC_REQ_STOP;
    svc_call(svcs[id].name, req, true);  // xxx dont wait ? then who frees?
    free(req);
}

static void process_svc_stopped_callback(int id, int rc)
{
    svc_t *x = &svcs[id];

    INFO("called for id=%d name=%s rc=%d\n", id, x->name, rc);

    x->state = (rc == 0 ? SERVICE_STATE_STOPPED : SERVICE_STATE_STOPPED_BY_ERROR);
}   

// -----------------  RUN A SVC  ------------------------------------

static int svc_thread(void *cx);

static void run_svc(int id)
{
    sdlx_create_detached_thread(svc_thread, (void*)(long)id);
}

static int svc_thread(void *cx)
{
    int id = (int)(long)cx;
    svc_t *x = &svcs[id];
    int rc;

    rc = run(x->name, id);  // xxx maybe doesnt need id

    process_svc_stopped_callback(id, rc);

    return 0;
}
