#include <std_hdrs.h>

#include <sdlx.h>
#include <utils.h>
#include <logging.h>

#ifdef ANDROID
#include <SDL3/SDL.h>
#endif

#include "version.h"

// xxx
// - update comments throughout

//
// defines
//

#ifdef ANDROID
#define MAIN SDL_main
#else
#define MAIN main
#endif

#define DEFAULT_DEVEL_PORT 9000   // IANA registered port range 1024 - 49151

#define LAST_PAGE ((max_apps - 1) / 18)

#define BG_COLOR (!params.devel_mode ? COLOR_TEAL : COLOR_VIOLET)

#define SMALLEST_FONT 40
#define SMALL_FONT    30
#define DEFAULT_FONT  20
#define LARGE_FONT    10

#define EVID_PAGE_DECREMENT  900
#define EVID_PAGE_INCREMENT  901
#define EVID_MINIMIZE        902

#define MS  1000
#define SEC 1000000

#define CREATE_FILES_INIT        1
#define CREATE_FILES_RESET_APPS  2
#define CREATE_FILES_RESET_SVCS  3

//
// typedefs
//

typedef struct {
    bool devel_mode;
    int  devel_port;
} params_t;

//
// variables
//

static char       *storage_path;
static params_t    params;
static pthread_t   server_tid;

//
// prototypes  xxx put all prototypes here ?
//

static void processing(void);

static int services_monitor_thread(void *cx);
static void stop_all_services(bool lock_held_by_caller);
static void request_start_all_autostart_services(bool lock_held_by_caller);
static void acquire_services_lock(void) __attribute__((unused));
static void release_services_lock(void) __attribute__((unused));

static int server_thread(void *cx);
static int waiter_thread(void *cx);
static void kill_child_processes(pid_t pid);

//
// routines to launch a C program using picoc interpreter
//

extern int picoc_ezapp(char *args);

// -----------------  MAIN  ------------------------------------------

static int init(void);
static void cleanup(void);
static void sigusr2_hndlr(int signum);
#ifdef ANDROID  // xxx get rid of some ifdefs
static void create_files(int action);
#endif

int MAIN(int argc, char **argv)
{
    int rc;

    rc = init();
    if (rc != 0) {
        return 1;
    }

    processing();

    cleanup();

    return 0;
}

static int init(void)
{
    int rc;

    // get storage_path, and
    // set current working directory to storage_path
    storage_path = sdlx_get_storage_path();
    chdir(storage_path);

    // init logging
    rc = log_init();
    if (rc != 0) {
        return -1;
    }

    // print startup message
    INFO("========== STARTING: %s %s  ==========\n", VERSION, BUILD_DATE);
    INFO("storage_path = %s\n", storage_path);

    // get params, if they don't exist, set to default value
    params.devel_mode = util_get_int_param(".", "devel_mode", 0);
    params.devel_port = util_get_int_param(".", "devel_port", DEFAULT_DEVEL_PORT);

    // xxx temporary for development
    //params.devel_mode = 1;
    //util_set_int_param(".", "devel_mode", 1);

#ifdef ANDROID
    // copy asset files to the working directory
    sdlx_copy_asset_file("files.tar", ".");

    // if apps dir doesn't exist then 
    // extract all from files.tar
    // xxx true is temporary
    if (true || !util_file_exists(".", "apps")) { 
        create_files(CREATE_FILES_INIT);
    }
#endif

    // allocate SIGUSR2, this signal is sent to the server_thread
    // when developer mode is disabled or developer mode port is changed
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = sigusr2_hndlr;
    sigaction(SIGUSR2, &action, NULL);

    // create server threads
    sdlx_create_detached_thread(server_thread, NULL);
    sdlx_create_detached_thread(waiter_thread, NULL);

    // init sdl xxx move
    sdlx_init(SUBSYS_VIDEO | SUBSYS_AUDIO | SUBSYS_SENSOR);
    INFO("sdlx_win_width,height = %d %d  sdlx_char_width,height=%d %d\n",
         sdlx_win_width, sdlx_win_height, sdlx_char_width, sdlx_char_height);

#ifdef ANDROID
    // get permissions when running on Android
    if (sdlx_get_permission("android.permission.POST_NOTIFICATIONS") != 0) {
        ERROR("failed to get permission POST_NOTIFICATION\n");
    }
    if (sdlx_get_permission("android.permission.ACCESS_COARSE_LOCATION") != 0) {
        ERROR("failed to get permission ACCESS_COARSE_LOCATION\n");
    }
    if (sdlx_get_permission("android.permission.ACCESS_FINE_LOCATION") != 0) {
        ERROR("failed to get permission ACCESS_FINE_LOCATION\n");
    }
    if (sdlx_get_permission("android.permission.ACTIVITY_RECOGNITION") != 0) {
        ERROR("failed to get permission ACTIVITY_RECOGNITION\n");
    }
#endif

    // start the services_monitor_thread
    request_start_all_autostart_services(false);
    sdlx_create_detached_thread(services_monitor_thread, NULL);

    // init okay
    return 0;
}

static void cleanup(void)
{
    INFO("TERMINATING\n");

    stop_all_services(false);

    kill_child_processes(getpid());

    sdlx_quit(SUBSYS_VIDEO | SUBSYS_AUDIO | SUBSYS_SENSOR);
}

#ifdef ANDROID
static void create_files(int action)
{
    switch (action) {
    case CREATE_FILES_INIT:
        system("tar -xvf files.tar");
        break;
    case CREATE_FILES_RESET_APPS:
        system("rm -rf apps apps_data");
        system("tar -xvf files.tar apps");
        system("mkdir -p apps_data");
        break;
    case CREATE_FILES_RESET_SVCS:
        acquire_services_lock();
        stop_all_services(true);

        system("rm -rf svcs svcs_data");
        system("tar -xvf files.tar svcs");
        system("mkdir -p svcs_data");

        request_start_all_autostart_services(true);
        release_services_lock();
        break;
    }
}
#endif

static void sigusr2_hndlr(int signum)
{
    // nothing needed here
}

// -----------------  PROCESSING  ------------------------------------

#define MAX_APPS 100

static char *apps[MAX_APPS];
static int   max_apps;
static int   page;

static int run(char *name, int svc_id);
static void display_menu(void);
static void get_list_of_apps(void);
static void settings(void);
static void services(void);

static void processing(void)
{
    sdlx_event_t event;

    while (true) {
        // clear the display, and set the font to default
        sdlx_display_init(BG_COLOR);
        sdlx_print_init(DEFAULT_FONT, COLOR_WHITE, BG_COLOR);

        // display menu, and register for events
        display_menu();

        // register for screen bottom control events
        if (LAST_PAGE > 0) {
            sdlx_register_control_events("<", ">", "X", BG_COLOR,
                                        EVID_PAGE_DECREMENT, EVID_PAGE_INCREMENT, EVID_MINIMIZE);
        } else {
            sdlx_register_control_events(NULL, NULL, "X", BG_COLOR, 0, 0, EVID_MINIMIZE);
        }

        // update the display
        sdlx_display_present();

        // wait for an event, 1 sec timeout
        sdlx_get_event(1*SEC, &event);
        if (event.event_id == -1) {
            continue;
        }

        // process the event
        INFO("proc event_id %d\n", event.event_id);
        if (event.event_id == EVID_QUIT) {
            break;
        } else if (event.event_id == EVID_MINIMIZE) {
            sdlx_minimize_window();
        } else if (event.event_id == EVID_PAGE_DECREMENT) {
            if (--page < 0) {
                page = LAST_PAGE;
            }
        } else if (event.event_id == EVID_PAGE_INCREMENT) {
            if (++page > LAST_PAGE) {
                page = 0;
            }
        } else if (event.event_id >= 0 && event.event_id <= max_apps-1) {
            int id = event.event_id;
            if (apps[id] == NULL) {
                ERROR("apps[%d] is NULL\n", id);
            } else if (strcmp(apps[id], "Settings") == 0) {
                settings();
            } else {
                run(apps[id], -1);
            }
        }
    }
}

// args:
// - name: the name of the app or svc
// - svc_id: use -1 for app; else the svc_id is >= 0, and will be passed 
//           in argv to the svc; the svc uses this value when polling for
//           svc_stop_requested
static int run(char *name, int svc_id)
{
    char           dir_path[100];
    char           data_dir_path[100];
    char           cmd[120];
    int            rc;
    DIR           *dir;
    struct dirent *dirent;
    char          *p;
    char           picoc_args[1000];

    // xxx comment
    if (svc_id == -1) {
        sprintf(dir_path, "apps/%s", name);
        sprintf(data_dir_path, "apps_data/%s", name);
    } else {
        sprintf(dir_path, "svcs/%s", name);
        sprintf(data_dir_path, "svcs_data/%s", name);
    }

    // xxx
    sprintf(cmd, "mkdir -p %s", data_dir_path);
    system(cmd);

    // construct list of *.c files in the dir
    picoc_args[0] = '\0';
    dir = opendir(dir_path);
    if (dir == NULL) {
        ERROR("%s: failed to opendir %s, %s\n", name, dir_path, strerror(errno));
        return 99;
    }
    p = picoc_args;
    while ((dirent = readdir(dir)) != NULL) {
        char *fn = dirent->d_name;
        int len = strlen(fn);
        if (len > 2 && strcmp(fn+len-2, ".c") == 0) {
            p += sprintf(p, "%s/%s ", dir_path, fn);
        }
    }
    closedir(dir);

    // error if no source code found in dir_path
    if (picoc_args[0] == '\0') {
        ERROR("%s: no source code in %s\n", name, dir_path);
        return 99;
    }

    // xxx comment
    if (svc_id == -1) {
        p += sprintf(p, " - %s %s", dir_path, data_dir_path);
    } else {
        p += sprintf(p, " - %s %s %d", dir_path, data_dir_path, svc_id);
    }

    // run the app using the picoc c language interpreter
    INFO("%s: starting, args = %s\n", name, picoc_args);
    rc = picoc_ezapp(picoc_args);  // xxx get rid of picoc_bg
    INFO("%s: completed, rc = %d\n", name, rc);

    // return completion status
    return rc;
}

// -----------------  DISPLAY MENU  -------------------------------

static void display_menu(void)
{
    static sdlx_texture_t *circle;
    int first, last;
    sdlx_print_state_t print_state;

    #define RADIUS 100

    // allocate circle texture, which is used when displaying menu items
    if (circle == NULL) {
        circle = sdlx_create_filled_circle_texture(RADIUS, COLOR_BLUE);
    }

    // get the list of apps: 
    // - this initializes the apps[] array of  app names
    // - the dir names must be the same as the app names
    // - the apps array is indexed by the location on the display, for
    //   example idx=0 is top left, and idx=17 is bottom right
    get_list_of_apps();

    first = page * 18;
    last  = first + 17;

    if (LAST_PAGE > 0) {
        sdlx_print_save(&print_state);
        sdlx_print_init(SMALL_FONT, COLOR_WHITE, BG_COLOR);
        sdlx_render_printf_xyctr(sdlx_win_width/2, sdlx_char_height/2, "Page %d", page);
        sdlx_print_restore(&print_state);
    }

    for (int i = first; i <= last; i++) {
        char     *name = apps[i];
        char      s1[10], s2[10];
        int       len, l1, l2, lmax, x, y;
        double    chw, chh, numchars;
        sdlx_loc_t loc;

        if (name == NULL) {
            continue;
        }

        len  = strlen(name);
        if (len > 8) len = 8;

        if (len <= 4) {
            l1 = len;
            l2 = 0;
            strcpy(s1, name);
            s2[0] = '\0';
            lmax = l1;
        } else {
            l1 = len / 2;
            l2 = len - l1;
            strncpy(s1, name, l1);
            strncpy(s2, name+l1, l2);
            s1[l1] = '\0';
            s2[l2] = '\0';
            lmax = l2;
        }

        if (s2[0] == '\0') {
            double k = (len == 1 ? 1 : 1.5);
            chw = (k * RADIUS) / lmax;
            numchars = sdlx_win_width / chw;
        } else {
            double k = ((len == 5 || len == 6) ? 1.35 : 1.5);
            chw = (k * RADIUS) / lmax;
            numchars = sdlx_win_width / chw;
        }
        chh = chw / 0.6;

        // determine dispaly location of the center of the menu item
        x = (sdlx_win_width/3/2) + (i%3) * (sdlx_win_width/3);
        y = ((sdlx_win_height-150)/6/2) + ((i-first)/3) * ((sdlx_win_height-150)/6);

        // display the menu item
        // - first render the circle
        // - then render the app name text within the circle
        sdlx_render_texture(x-RADIUS, y-RADIUS, -1, -1,  0, circle);
        sdlx_print_init(numchars, COLOR_WHITE, COLOR_BLUE);
        if (s2[0] == '\0') {
            sdlx_render_text_xyctr(x, y, s1);
        } else {
            sdlx_render_text_xyctr(x, nearbyint(y-0.5*chh), s1);
            sdlx_render_text_xyctr(x, nearbyint(y+0.5*chh), s2);
        }

        // register event
        loc.x = x - RADIUS;
        loc.y = y - RADIUS;
        loc.w = 2 * RADIUS;
        loc.h = 2 * RADIUS;
        sdlx_register_event(&loc, i);
    }
}

static void get_list_of_apps(void)
{
    const char *layout_file_path = "apps/layout";
    struct stat statbuf;
    int         rc, i, cnt, secs, n, line_num;
    FILE       *fp;
    char        str[200], s[3][100];

    static long layout_file_mtime;
    static bool first_call = true;

    // if layout file doesn't exist then
    // return without changing the list of apps,
    // because the file may have just been temporarily deleted
    rc = stat(layout_file_path, &statbuf);
    if (rc != 0) {
        return;
    }

    // if layout file has not changed then 
    // return without updating the list of apps
    if (statbuf.st_mtime == layout_file_mtime) {
        return;
    }
    layout_file_mtime = statbuf.st_mtime;

    // obtain the list of apps from the layout file ...

    // the layut file may currently being updated;
    // wait for the layout file to not have any processes having it open
    if (!first_call) {
        secs = 0;
        while (true) {
            str[0] = '\0';
            fp = popen("lsof apps/layout | wc -l", "r");
            fgets(str, sizeof(str), fp);
            pclose(fp);
            cnt = sscanf(str, "%d", &n);
            if (cnt != 1) {
                ERROR("invalid output from wc, '%s'\n", str);
                break;
            }
            if (n == 0) {
                INFO("layout file not open by any processes, secs=%d\n", secs);
                break;
            }
            if (secs >= 3) {
                ERROR("timedout waiting for layout file\n");
                break;
            }
            sleep(1);
            secs++;
        }
    }
    first_call = false;

    // free the current apps names
    for (i = 0; i < max_apps; i++) {
        free(apps[i]);
        apps[i] = NULL;
    }
    max_apps = 0;

    // read the app names, which must be the same as their dir names, from the layout file
    line_num = 0;
    fp = fopen(layout_file_path, "r");
    while (fgets(str, sizeof(str), fp)) {
        line_num++;

        // xxx cleanup input str by removing terminating newline 
        // and removing leading spaces

        // ignore lines that are blank or begin with comment char
        if (str[0] == '\n' || str[0] == '#') {
            continue;
        }

        // read 3 app names from each line of the layout file
        cnt = sscanf(str, "%s %s %s", s[0], s[1], s[2]);
        if (cnt != 3) {
            ERROR("invalid line, line_num = %d\n", line_num);
            break;
        }

        // store the app names, just read, to the apps[] array;
        // ignoring app names that are "-"
        for (i = 0; i < 3; i++) {
            if (strcmp(s[i], "-") != 0) {
                apps[max_apps] = strdup(s[i]);
            }
            max_apps++;
        }
    }
    fclose(fp);

    // debug print the list of apps names
    INFO("max_apps = %d\n", max_apps);
    for (i = 0; i < max_apps; i++) {
        if (apps[i] != NULL) {
            INFO("apps[%d] = %s\n", i, apps[i]);
        }
    }
}

// -----------------  SERVICES  -----------------------------------

// xxx 
// - comments
// - full review

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

#define MAX_SERVICES 100 // xxx dont need this many

// xxx should this use SDL Mutex
#define LOCK do { pthread_mutex_lock(&services_mutex); } while (0)
#define UNLOCK do { pthread_mutex_unlock(&services_mutex); } while (0)

typedef struct {
    char *name;
    int   state;
    bool  delete_pending;
} service_t;

static pthread_mutex_t services_mutex = PTHREAD_MUTEX_INITIALIZER;
static service_t       services_tbl[MAX_SERVICES];
static char           *svcs[MAX_SERVICES];
static int             max_svcs;
static bool            start_autostart_services_req_flag;

extern char stop_requested[MAX_SERVICES];

static void process_changed_svc_names(void);
static void process_start_req(int id);
static void process_stop_req(int id);
static void process_stopped_callback(int id, int rc);
static void run_svc(int id);
static void get_list_of_svcs(bool *changed);
static void start_autostart_services(void);
static int alloc_service(char *name);
static void free_service(int id);
static bool is_name_in_svcs(char *name);
static bool is_name_in_services(char *name);

static void acquire_services_lock(void)
{
    LOCK;
}

static void release_services_lock(void)
{
    UNLOCK;
}

static void request_start_all_autostart_services(bool lock_held_by_caller)
{
    if (!lock_held_by_caller) {
        LOCK;
    }

    start_autostart_services_req_flag = true;

    if (!lock_held_by_caller) {
        UNLOCK;
    }
}

static int services_monitor_thread(void *cx)
{
    bool changed;

    while (true) {
        LOCK;

        get_list_of_svcs(&changed);
        if (changed) {
            process_changed_svc_names();
        }

        if (start_autostart_services_req_flag) {
            start_autostart_services();
            start_autostart_services_req_flag = false;
        }

        UNLOCK;

        sleep(1);  // xxx how long to sleep?
    }

    return 0;
}

static void services(void)
{
    sdlx_event_t event;
    int         id;
    bool        done = false;
    sdlx_loc_t  *loc;
    double      row;

    // xxx use MAX_SERVICES in these defines
    #define EVID_SVC_START    100
    #define EVID_SVC_STOP     200
    #define EVID_SVC_RESTART  300

    LOCK;

    // handle the setting display
    while (true) {
        // init display and display title line
        sdlx_display_init(BG_COLOR);
        sdlx_print_init(DEFAULT_FONT, COLOR_WHITE, BG_COLOR);
        sdlx_render_text_xyctr(sdlx_win_width/2, sdlx_char_height/2, "Services");

        // display name and controls for each service
        row = 2;
        for (id = 0; id < MAX_SERVICES; id++) {
            service_t *x = &services_tbl[id];

            if (x->name == NULL) {
                continue;
            }
        
            sdlx_print_init_color(SERVICE_STATE_TO_COLOR(x->state), BG_COLOR);
            sdlx_render_printf(0, ROW2Y(row), "%-s", x->name);

            sdlx_print_init_color(COLOR_LIGHT_BLUE, BG_COLOR);
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
        sdlx_register_control_events(NULL, NULL, "X", BG_COLOR, 0, 0, EVID_QUIT);

        // present the display
        sdlx_display_present();

        // wait for an event, with 100 ms timeout;
        // if no event received then re-display
        UNLOCK;
        sdlx_get_event(100*MS, &event);
        LOCK;
        if (event.event_id == -1) {
            continue;
        }

        // process the event
        INFO("proc event_id %d\n", event.event_id);
        switch (event.event_id) {
        case EVID_SVC_START ... EVID_SVC_START + MAX_SERVICES - 1:
            id = event.event_id - EVID_SVC_START;
            process_start_req(id);
            break;
        case EVID_SVC_STOP ... EVID_SVC_STOP + MAX_SERVICES - 1:
            id = event.event_id - EVID_SVC_STOP;
            process_stop_req(id);
            break;
        case EVID_QUIT:
            done = true;
            break;
        }

        if (done) {
            break;
        }
    }

    UNLOCK;
}

// - - - - - - - - - process event routines  - - - - - - - - - - - - - 

// lock held by caller
static void process_changed_svc_names(void)
{
    int id, i;

    INFO("called\n");

    // loop over all services;
    // if not in list of svcs directories then stop and delete it
    for (id = 0; id < MAX_SERVICES; id++) {
        service_t *x = &services_tbl[id];
        if (x->name == NULL) {
            continue;
        }
        if (is_name_in_svcs(x->name) == false) {
            if (x->state == SERVICE_STATE_RUNNING) {
                x->state = SERVICE_STATE_STOPPING;
                stop_requested[id] = true;
                x->delete_pending = true;
            } else if (x->state == SERVICE_STATE_STOPPING) {
                x->delete_pending = true;
            } else {
                free_service(id);
            }
        }
    }

    // loop over all names in svcs table;
    // if not in services_tbl then add it
    for (i = 0; i < max_svcs; i++) {
        char *name = svcs[i];
        if (is_name_in_services(name) == false) {
            alloc_service(name);
        }
    }
}

// lock held by caller
static void process_start_req(int id)
{
    service_t *x = &services_tbl[id];

    INFO("called for id=%d name=%s\n", id, x->name);

    if ((x->name == NULL) || !SERVICE_IS_STOPPED(x->state)) {
        ERROR("id=%d name=%s state=%s\n", id, x->name, SERVICE_STATE_STR(x->state));
        return;
    }
    
    x->state = SERVICE_STATE_RUNNING;
    stop_requested[id] = false;
    run_svc(id);
}

// lock held by caller
static void process_stop_req(int id)
{
    service_t *x = &services_tbl[id];

    INFO("called for id=%d name=%s\n", id, x->name);

    if (x->name == NULL || x->state != SERVICE_STATE_RUNNING) {
        ERROR("id=%d name=%s state=%s\n", id, x->name, SERVICE_STATE_STR(x->state));
        return;
    }

    x->state = SERVICE_STATE_STOPPING;
    stop_requested[id] = true;
}

static void process_stopped_callback(int id, int rc)
{
    service_t *x = &services_tbl[id];

    LOCK;

    INFO("called for id=%d name=%s rc=%d\n", id, x->name, rc);

    if (x->name == NULL) {
        ERROR("service id %d does not exist\n", id);
        UNLOCK;
        return;
    }

    x->state = (rc == 0 ? SERVICE_STATE_STOPPED : SERVICE_STATE_STOPPED_BY_ERROR);

    stop_requested[id] = false;

    if (x->delete_pending) {
        x->delete_pending = false;
        free_service(id);
    }

    UNLOCK;
}   

// - - - - - - - - - run the svc - - - - - - - - - - - - - - 

static int service_thread(void *cx);

// lock held by caller
static void run_svc(int id)
{
    sdlx_create_detached_thread(service_thread, (void*)(long)id);
}

static int service_thread(void *cx)
{
    int id = (int)(long)cx;
    service_t *x = &services_tbl[id];
    int rc;

    rc = run(x->name, id);

    process_stopped_callback(id, rc);

    return 0;
}

// - - - - - - - - - get list of svcs - - - - - - - - - - 

static int compare(const void *a_arg, const void *b_arg)
{
    char *a = *(char**)a_arg;
    char *b = *(char**)b_arg;
    return strcmp(a,b);
}

// lock held by caller
static void get_list_of_svcs(bool *changed)
{
    int            rc;
    struct stat    statbuf;
    DIR           *dir;
    struct dirent *dirent;
    char          *svcs_dir_path = "svcs";

    static long svcs_dir_mtime;

    // preset return flag
    *changed = false;

    // if svcs dir doesn't exist then return without changing list of svcs
    rc = stat(svcs_dir_path, &statbuf);
    if (rc != 0) {
        return;
    }

    // if svcs dir has not changed then return
    if (statbuf.st_mtime == svcs_dir_mtime) {
        return;
    }
    svcs_dir_mtime = statbuf.st_mtime;

    // free the current svcs names
    for (int i = 0; i < max_svcs; i++) {
        free(svcs[i]);
        svcs[i] = NULL;
    }
    max_svcs = 0;

    // make list of svcs subdirs
    dir = opendir(svcs_dir_path);
    if (dir == NULL) {
        ERROR("opendir %s failed, %s\n", svcs_dir_path, strerror(errno));
        return;
    }
    while ((dirent = readdir(dir)) != NULL) {
        char *name = dirent->d_name;
        int   type = dirent->d_type;
        if (type == DT_DIR && name[0] != '.') {
            svcs[max_svcs++] = strdup(name);
        }
    }
    closedir(dir);

    // sort the svcs names
    qsort(svcs, max_svcs, sizeof(char*), compare);

    // debug print the list of svcs
    INFO("max_svcs = %d\n", max_svcs);
    for (int i = 0; i < max_svcs; i++) {
        INFO("svcs[%d] = %s\n", i, svcs[i]);
    }

    // return flag indicating that there may have been
    // changes to the list of svc names
    *changed = true;
}

// - - - - - - - - - support routines - - - - - - - - - - - - -

static void stop_all_services(bool lock_held_by_caller)
{
    int id, duration_ms = 0;
    bool all_stopped;

    if (!lock_held_by_caller) {
        LOCK;
    }

    INFO("stopping all services\n");

    for (id = 0; id < MAX_SERVICES; id++) {
        service_t *x = &services_tbl[id];
        if (x->name && x->state == SERVICE_STATE_RUNNING) {
            stop_requested[id] = true;
        }
    }

    while (true) {
        all_stopped = true;
        for (id = 0; id < MAX_SERVICES; id++) {
            service_t *x = &services_tbl[id];
            if (x->name && !SERVICE_IS_STOPPED(x->state)) {
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
            for (id = 0; id < MAX_SERVICES; id++) {
                service_t *x = &services_tbl[id];
                if (x->name && !SERVICE_IS_STOPPED(x->state)) {
                    ERROR("- %-12s %s\n", x->name, SERVICE_STATE_STR(x->state));
                }
            }
            break;
        }

        usleep(100*MS);
        duration_ms += 100;
    }

    if (!lock_held_by_caller) {
        UNLOCK;
    }
}

// lock held by caller
static void start_autostart_services(void)
{
    int id;
    char svc_dir[100];

    for (id = 0; id < MAX_SERVICES; id++) {
        service_t *x = &services_tbl[id];
        if (x->name == NULL) {
            continue;
        }

        if (!SERVICE_IS_STOPPED(x->state)) {
            continue;
        }

        sprintf(svc_dir, "svcs/%s", x->name);
        if (!util_file_exists(svc_dir, "autostart")) {
            continue;
        }

        INFO("autostarting service %s\n", x->name);
        x->state = SERVICE_STATE_RUNNING;
        stop_requested[id] = false;
        run_svc(id);
    }
}

// lock held by caller
static int alloc_service(char *name)
{
    int id;

    for (id = MAX_SERVICES-1; id > 0; id--) {
        if (services_tbl[id].name == NULL && services_tbl[id-1].name != NULL) {
            break;
        }
    }

    if (services_tbl[id].name != NULL) {
        return -1;
    }

    INFO("allocating %d\n", id);
    service_t *x = &services_tbl[id];
    memset(x, 0, sizeof(service_t));
    x->name = strdup(name);

    return id;
}

// lock held by caller
static void free_service(int id)
{
    service_t *x = &services_tbl[id];

    free(x->name);
    x->name = NULL;
    memset(x, 0, sizeof(service_t));
}

// lock held by caller
static bool is_name_in_svcs(char *name)
{
    for (int i = 0; i < max_svcs; i++) {
        if (strcmp(svcs[i], name) == 0) {
            return true;
        }
    }

    return false;
}

// lock held by caller
static bool is_name_in_services(char *name)
{
    for (int id = 0; id < MAX_SERVICES; id++) {
        service_t *x = &services_tbl[id];
        if (x->name && strcmp(name, x->name) == 0) {
            return true;
        }
    }

    return false;
}

// -----------------  SETTINGS  -----------------------------------

static void copyright(void);

static void settings(void)
{
    sdlx_event_t event;
    sdlx_loc_t  *loc;
    bool        done = false;
    char       *msg = NULL;
    long        msg_time = 0;
    char       *ipaddr;

    #define EVID_COPYRIGHT   1001
    #define EVID_DEVEL_MODE  1002
    #define EVID_DEVEL_PORT  1003
    #define EVID_SERVICES    1004
#ifdef ANDROID
    #define EVID_RESET_APPS  1005
    #define EVID_RESET_SVCS  1006
#endif

    // get this device ipaddr
    ipaddr = util_get_ipaddr();
    INFO("SETTINGS %s:%d\n", ipaddr, params.devel_port);

    // handle the setting display
    while (true) {
        // init display and display title line
        sdlx_display_init(BG_COLOR);
        sdlx_print_init(DEFAULT_FONT, COLOR_WHITE, BG_COLOR);
        sdlx_render_text_xyctr(sdlx_win_width/2, sdlx_char_height/2, "Settings");

        // display version
        sdlx_render_printf(0, ROW2Y(2), "Version = %s", VERSION);
        sdlx_render_printf(0, ROW2Y(3), "%s", BUILD_DATE);

        // init print color to COLOR_LIGHT_BLUE for the following,
        // because these all are selectable
        sdlx_print_init_color(COLOR_LIGHT_BLUE, BG_COLOR);

        // display Copyright
        loc = sdlx_render_printf(0, ROW2Y(5), "Copyright");
        sdlx_register_event(loc, EVID_COPYRIGHT);

        // display Devel_Mode
        loc = sdlx_render_printf(0, ROW2Y(7), "Devel_Mode = %s", params.devel_mode ? "ON" : "OFF");
        sdlx_register_event(loc, EVID_DEVEL_MODE);

        // display Devel_Port
        loc = sdlx_render_printf(0, ROW2Y(9), "Devel_Port = %d", params.devel_port);
        sdlx_register_event(loc, EVID_DEVEL_PORT);

        // display Services
        loc = sdlx_render_printf(0, ROW2Y(11), "Services");
        sdlx_register_event(loc, EVID_SERVICES);

#ifdef ANDROID
        // display Reset_Apps
        loc = sdlx_render_printf(0, ROW2Y(13), "Reset_Apps");
        sdlx_register_event(loc, EVID_RESET_APPS);

        // display Reset_Svcs
        loc = sdlx_render_printf(0, ROW2Y(15), "Reset_Svcs");
        sdlx_register_event(loc, EVID_RESET_SVCS);
#endif

        // change print color back to white
        sdlx_print_init_color(COLOR_WHITE, BG_COLOR);

        // if a message is requested for display then do so;
        // otherwise, when in developer mode, display ipaddr:port
        if (msg && (util_microsec_timer() - msg_time) < 3000000) {
            sdlx_render_printf(0, sdlx_win_height-400, "%s", msg);
        } else if (params.devel_mode) {
            sdlx_render_printf(0, sdlx_win_height-400, "%s:%d", ipaddr, params.devel_port);
        }

        // display the control event 'X' to exit this screen
        sdlx_register_control_events(NULL, NULL, "X", BG_COLOR, 0, 0, EVID_QUIT);

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
        case EVID_COPYRIGHT:
            copyright();
            break;
        case EVID_DEVEL_MODE:
            params.devel_mode = (params.devel_mode ? 0 : 1);
            util_set_int_param(".", "devel_mode", params.devel_mode);
            if (!params.devel_mode) {
                INFO("sending SIGUSR2 to server_thread\n");
                pthread_kill(server_tid, SIGUSR2);
            }
            break;
        case EVID_DEVEL_PORT: {
            char *str; 
            int cnt, port;
            str = sdlx_get_input_str("Port?", true, BG_COLOR);
            cnt = sscanf(str, "%d", &port);
            if (cnt == 1 && (port >= 1024 && port <= 49151)) {
                params.devel_port = port;
                util_set_int_param(".", "devel_port", port);
                if (params.devel_mode) {
                    INFO("sending SIGUSR2 to server_thread\n");
                    pthread_kill(server_tid, SIGUSR2);
                }
            }
            break; }
        case EVID_SERVICES:
            services();
            break;
#ifdef ANDROID
        case EVID_RESET_APPS: {
            char *str; 
            str = sdlx_get_input_str("Reset y/n?", false, BG_COLOR);
            if (strcasecmp(str, "y") != 0) {
                break;
            }
            create_files(CREATE_FILES_RESET_APPS);
            msg = "Apps are reset";
            msg_time = util_microsec_timer();
            break; }
        case EVID_RESET_SVCS: {
            char *str; 
            str = sdlx_get_input_str("Reset y/n?", false, BG_COLOR);
            if (strcasecmp(str, "y") != 0) {
                break;
            }
            create_files(CREATE_FILES_RESET_SVCS);
            msg = "Svcs are reset";
            msg_time = util_microsec_timer();
            break; }
#endif
        case EVID_QUIT:
            done = true;
            break;
        }

        if (done) {
            break;
        }
    }
}

static void copyright(void)
{
    char       *str;
    int         y_display_begin, y_display_end, y_top;
    int         len;
    sdlx_event_t event;
    bool        done = false;

    // read the copyright file
    str = util_read_file(".", "copyright", &len);
    if (str == NULL) {
        ERROR("failed to read copyright file\n");
        return;
    }

    // init vars
    y_display_begin = 100;
    y_display_end = sdlx_win_height - 200;
    y_top = y_display_begin;

    // display copyright, support motion (for scrolling)
    while (true) {
        // display copyright and register for motion (scrolling) & exit events
        sdlx_display_init(BG_COLOR);
        sdlx_print_init(SMALLEST_FONT, COLOR_WHITE, BG_COLOR);
        sdlx_register_event(NULL, EVID_MOTION);
        sdlx_render_multiline_text(y_top, y_display_begin, y_display_end, str);
        sdlx_register_control_events(NULL, NULL, "X", BG_COLOR, 0, 0, EVID_QUIT);
        sdlx_display_present();

        sdlx_get_event(-1, &event);
        switch (event.event_id) {
        case EVID_MOTION:
            y_top += event.u.motion.yrel;
            if (y_top >= y_display_begin) {
                y_top = y_display_begin;
            }
            break;
        case EVID_QUIT:
            done = true;
            break;
        }

        if (done) {
            break;
        }
    }

    // free allocated copyrght buffer
    free(str);
}

// ----------------- SERVER ----------------------------

#define MAX_PID_TBL 20

static int process_req_thread(void *cx);
static void process_req_using_android_sh(int sockfd, char *cmd);

static int server_thread(void *cx)
{
    struct sockaddr_in server_address;
    int                listen_sockfd, ret;

    // save server thread id in global, so that signals can be sent to this thread
    server_tid = pthread_self();

    sleep(5); //xxx
    double lat, lng, alt;
    util_get_location(&lat, &lng, &alt);  // return 9999 if not bound
    INFO("XXXXXXXXXXXXXXX %f %f %f\n", lat, lng, alt);

again:
    // wait for developer mode to be enabled
    INFO("waiting for devel_mode enabled\n");
    sleep(1);
    while (params.devel_mode == false) {
        sleep(1);
    }
    INFO("server starting, listening on port %d\n", params.devel_port);

    // create listen socket
    listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sockfd == -1) {
        ERROR("socket, %s\n", strerror(errno));
        return 0;
    }

    // set socket options
    int reuseaddr = 1;
    ret = setsockopt(listen_sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseaddr, sizeof(reuseaddr));
    if (ret == -1) {
        ERROR("setsockopt SO_REUSEADDR, %s\n", strerror(errno));
        return 0;
    }

    // bind socket to any ip addr, for specified port
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family      = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port        = htons(params.devel_port);
    ret = bind(listen_sockfd,
               (struct sockaddr *)&server_address,
               sizeof(server_address));
    if (ret == -1) {
        ERROR("bind, %s\n", strerror(errno));
        close(listen_sockfd);
        sleep(1);
        goto again;
    }

    // listen 
    ret = listen(listen_sockfd, 5);
    if (ret == -1) {
        ERROR("listen, %s\n", strerror(errno));
        return 0;
    }

    // accept and process connections
    INFO("accepting connections\n");
    while (1) {
        int                sockfd;
        struct sockaddr_in peer_addr;
        socklen_t          peer_addr_len;

        // accept connection
        peer_addr_len = sizeof(peer_addr);
        sockfd = accept(listen_sockfd, (struct sockaddr *) &peer_addr, &peer_addr_len);
        if (sockfd == -1) {
            ERROR("accept, %s\n", strerror(errno));
            break;
        }

        // create thread to process the client request
        sdlx_create_detached_thread(process_req_thread, (void*)(long)sockfd);
    }

    // close listen socket
    close(listen_sockfd);

    // kill all child processes
    kill_child_processes(getpid());

    // goto top to reinit server_thread
    goto again;

    // not reached
    INFO("SERVER_THREAD TERMINATING\n");
    return 0;
}

static int process_req_thread(void *cx)
{
    int sockfd = (int)(long)cx;
    pid_t pid;

    char cmd[1000], *p;

    // read first line from sockfd, this contains the cmd to execute
    p = cmd;
    while (true) {
        char ch;
        int ret;
        ret = read(sockfd, &ch, 1);
        if (ret != 1) {
            ERROR("failed to read ch from sockfd %d, %s\n", sockfd, strerror(errno));
            close(sockfd);
            return 0;
        }
        if (ch == '\n') {
            break;
        }
        *p++ = ch;
    }
    *p = '\0';

    // fork and exec /bin/sh, to execute cmd
    if ((pid = fork()) == 0) {
        process_req_using_android_sh(sockfd, cmd);
    }
    INFO("created pid %d, cmd='%s'\n", pid, cmd);

    // parent is done with sockfd
    close(sockfd);
    return 0;
}

static void process_req_using_android_sh(int sockfd, char *cmd)
{
    char *argv[10];
    char cmd2[1100];

    // execute the cmd
    close(0);
    close(1);
    close(2);

    dup2(sockfd, 0);
    dup2(sockfd, 1);
    dup2(sockfd, 2);

    sprintf(cmd2, "cd %s; %s", storage_path, cmd);
    argv[0] = "/bin/sh";
    argv[1] = "-c";
    argv[2] = cmd2;
    argv[3] = NULL;

    execv("/bin/sh", argv);

    // not reached
    exit(1);
}

static int waiter_thread(void *cx)
{
    pid_t pid;

    while (true) {
        // wait for a process to terminate
        pid = wait(NULL);

        // it is normal for the above call to wait to return an
        // error when there are no child processes
        if (pid == -1) {
            if (errno != ECHILD) {
                ERROR("wait failed, %s\n", strerror(errno));
            }
            sleep(1);
            continue;
        }
    }

    return 0;
}

static void kill_child_processes(pid_t pid)
{
    FILE *fp;
    pid_t child_pid;
    char cmd[100], s[100];

    // use ps to get the child pid(s)
    sprintf(cmd, "ps -o pid= --ppid %d", pid);
    fp = popen(cmd, "r");
    while (fgets(s, sizeof(s), fp) != NULL) {
        if (sscanf(s, "%d", &child_pid) == 1) {
            kill_child_processes(child_pid);
        }
    }
    pclose(fp);

    // dont kill self
    if (pid != getpid()) {
        char cmdline_path[50], cmdline[80];
        int  fd, len, i;
        bool got_cmdline = false;

        // debug print the cmdline of pid that is about to be killed
        memset(cmdline, 0, sizeof(cmdline));
        sprintf(cmdline_path, "/proc/%d/cmdline", pid);
        fd = open(cmdline_path, O_RDONLY);
        if (fd >= 0) {
            len = read(fd, cmdline, sizeof(cmdline)-1);
            if (len > 0) {
                for (i = 0; i < len; i++) {
                    if (cmdline[i] == '\0') cmdline[i] = ' ';
                }
                got_cmdline = true;
            } else {
                sprintf(cmdline, "failed read %s", cmdline_path);
            }
            close(fd);
        } else {
            sprintf(cmdline, "failed open %s", cmdline_path);
        }

        // kill pid
        if (got_cmdline) {
            INFO("killing pid=%d: %s\n", pid, cmdline);
            kill(pid,SIGKILL);
        }
    }
}
