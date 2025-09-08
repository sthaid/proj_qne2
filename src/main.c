#include <std_hdrs.h>

#include <sdl.h>
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

#define DEVEL
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

#define TEN_MS  10000
#define ONE_SEC 1000000

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

//
// prototypes 
//

static void processing(void);
static int server_thread(void *cx);
static int waiter_thread(void *cx);
static void kill_child_processes(pid_t pid);

//
// routines to launch a C program using picoc interpreter
//

extern int picoc_fg(char *args);
extern void picoc_bg(char *args);

#ifdef ANDROID
extern void showHome(void); //xxx names, etc
extern void showHome2(void);
#endif

// -----------------  MAIN  ------------------------------------------

static int init(void);
static void cleanup(void);
static void create_default_apps(void); //xxx and services
static void sigusr1_hndlr(int signum);  //xxx sigusr1,  ?  sigusr2

int MAIN(int argc, char **argv)
{
    int rc;

    rc = init();
    if (rc != 0) {
        // xxx check all error return paths, should print
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
    storage_path = sdl_get_storage_path();
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

    // copy asset files to the working directory
    sdl_copy_asset_file("apps.tar", ".");
    sdl_copy_asset_file("copyright", ".");
    sdl_copy_asset_file("FreeMonoBold.ttf", ".");

#ifdef DEVEL
    // for development:
    // - enable developer mode
    // - init apps to default
    params.devel_mode = 1;
    util_set_int_param(".", "devel_mode", 1);
    create_default_apps();
#else
    // if apps dir struct doesn't exist then create it
    int rc;
    struct stat statbuf;
    rc = stat("apps", &statbuf);
    if (rc != 0 || !S_ISDIR(statbuf.st_mode)) {
        create_default_apps();
    }
#endif

    // allocate SIGUSR2, this signal is sent to the server_thread
    // when developer mode is disabled or developer mode port is changed
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = sigusr1_hndlr;
    sigaction(SIGUSR2, &action, NULL);

    // create server threads
    sdl_create_detached_thread(server_thread, NULL);
    sdl_create_detached_thread(waiter_thread, NULL);

    // init sdl
    sdl_init();
    INFO("sdl_win_width,height = %d %d  sdl_char_width,height=%d %d\n",
         sdl_win_width, sdl_win_height, sdl_char_width, sdl_char_height);

#ifdef ANDROID
    // xxx
    if (sdl_get_permission("android.permission.POST_NOTIFICATION") != 0) {
        ERROR("failed to get permission POST_NOTIFICATION\n");
    }
    if (sdl_get_permission("android.permission.ACCESS_COARSE_LOCATION") != 0) {
        ERROR("failed to get permission ACCESS_COARSE_LOCATION\n");
    }
    if (sdl_get_permission("android.permission.ACCESS_FINE_LOCATION") != 0) {
        ERROR("failed to get permission ACCESS_FINE_LOCATION\n");
    }

    // xxx
    showHome();
#endif

    // init okay
    return 0;
}

static void cleanup(void)
{
    INFO("TERMINATING\n");

//#ifdef ANDROID
//    showHome2(); //xxx
//#endif

    kill_child_processes(getpid());

    sdl_exit();
}

// xxx and services
static void create_default_apps(void)
{
    int rc;

    // remove existing apps dir struct
    rc = system("rm -rf apps");
    if (rc != 0) {
        ERROR("rm -rf apps, failed\n");
    }

    // extract apps.tar
    rc = system("tar -xvf apps.tar");
    if (rc != 0) {
        ERROR("tar -xvf apps.tar, failed\n");
    }
}

static void sigusr1_hndlr(int signum)
{
    // nothing needed here
}

// -----------------  PROCESSING  ------------------------------------

#define MAX_APPS 100

static char *apps[MAX_APPS];
static int   max_apps;
static int   page;

static int run_app(char *name, int svc_id);
static void display_menu(void);
static void get_list_of_apps(void);
static void settings(void);
static void services(void);

static void processing(void)
{
    sdl_event_t event;

    while (true) {
        // clear the display, and set the font to default
        sdl_display_init(BG_COLOR);
        sdl_print_init(DEFAULT_FONT, COLOR_WHITE, BG_COLOR);

        // display menu, and register for events
        display_menu();

        // register for screen bottom control events
        if (LAST_PAGE > 0) {
            sdl_register_control_events("<", ">", "X", BG_COLOR,
                                        EVID_PAGE_DECREMENT, EVID_PAGE_INCREMENT, EVID_MINIMIZE);
        } else {
            sdl_register_control_events(NULL, NULL, "X", BG_COLOR, 0, 0, EVID_MINIMIZE);
        }

        // update the display
        sdl_display_present();

        // wait for an event, 1 sec timeout
        sdl_get_event(ONE_SEC, &event);
        if (event.event_id == -1) {
            continue;
        }

        // process the event
        INFO("proc event_id %d\n", event.event_id);
        if (event.event_id == EVID_QUIT) {
            break;
        } else if (event.event_id == EVID_MINIMIZE) {
            sdl_minimize_window();
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
            } else if (strcmp(apps[id], "Services") == 0) {
                services();
            } else {
                run_app(apps[id], -1);
            }
        }
    }
}

//xxx make this a common routine
static int run_app(char *name, int svc_id)
{
    char           app_dir[100];
    int            rc;
    DIR           *dir;
    struct dirent *dirent;
    char          *p;

    static char picoc_args[1000];

    // construct list of *.c files in the app_dir
    sprintf(app_dir, "apps/%s", name);
    picoc_args[0] = '\0';
    dir = opendir(app_dir);
    p = picoc_args;
    while ((dirent = readdir(dir)) != NULL) {
        char *fn = dirent->d_name;
        int len = strlen(fn);
        if (len > 2 && strcmp(fn+len-2, ".c") == 0) {
            p += sprintf(p, "%s/%s ", app_dir, fn);
        }
    }
    closedir(dir);

    // error if no source code found in app_dir
    if (picoc_args[0] == '\0') {
        ERROR("%s: no source code in %s\n", name, app_dir);
        return 99;
    }

    // xxx comment
    if (svc_id == -1) {
        p += sprintf(p, " - %s", app_dir);
    } else {
        p += sprintf(p, " - %s %d", app_dir, svc_id);
    }

    // run the app using the picoc c language interpreter
    INFO("%s: starting, args = %s\n", name, picoc_args);
    rc = picoc_fg(picoc_args);  // xxx get rid of picoc_bg
    INFO("%s: completed, rc = %d\n", name, rc);

    // return completion status
    return rc;
}

// -----------------  DISPLAY MENU  -------------------------------

static void display_menu(void)
{
    static sdl_texture_t *circle;
    int first, last;
    sdl_print_state_t print_state;

    #define RADIUS 100

    // allocate circle texture, which is used when displaying menu items
    if (circle == NULL) {
        circle = sdl_create_filled_circle_texture(RADIUS, COLOR_BLUE);
    }

    // get the list of apps: 
    // - this initializes the apps[] array of  app names
    // - the dir names are the same as the app names
    // - the apps array is indexed by the location on the display, for
    //   example idx=0 is top left, and idx=17 is bottom right
    get_list_of_apps();

    first = page * 18;
    last  = first + 17;

    if (LAST_PAGE > 0) {
        sdl_print_save(&print_state);
        sdl_print_init(SMALL_FONT, COLOR_WHITE, BG_COLOR);
        sdl_render_printf_xyctr(sdl_win_width/2, sdl_char_height/2, "Page %d", page);
        sdl_print_restore(&print_state);
    }

    for (int i = first; i <= last; i++) {
        char     *name = apps[i];
        char      s1[10], s2[10];
        int       len, l1, l2, lmax, x, y;
        double    chw, chh, numchars;
        sdl_loc_t loc;

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
            numchars = sdl_win_width / chw;
        } else {
            double k = ((len == 5 || len == 6) ? 1.35 : 1.5);
            chw = (k * RADIUS) / lmax;
            numchars = sdl_win_width / chw;
        }
        chh = chw / 0.6;

        // determine dispaly location of the center of the menu item
        x = (sdl_win_width/3/2) + (i%3) * (sdl_win_width/3);
        y = ((sdl_win_height-150)/6/2) + ((i-first)/3) * ((sdl_win_height-150)/6);

        // display the menu item
        // - first render the circle
        // - then render the app name text within the circle
        sdl_render_texture(x-RADIUS, y-RADIUS, -1, -1,  0, circle);
        sdl_print_init(numchars, COLOR_WHITE, COLOR_BLUE);
        if (s2[0] == '\0') {
            sdl_render_text_xyctr(x, y, s1);
        } else {
            sdl_render_text_xyctr(x, nearbyint(y-0.5*chh), s1);
            sdl_render_text_xyctr(x, nearbyint(y+0.5*chh), s2);
        }

        // register event
        loc.x = x - RADIUS;
        loc.y = y - RADIUS;
        loc.w = 2 * RADIUS;
        loc.h = 2 * RADIUS;
        sdl_register_event(&loc, i);
    }
}

static void get_list_of_apps(void)
{
    const char *layout_file_path = "apps/layout";
    struct stat statbuf;
    int         rc, i, cnt;
    FILE       *fp;
    char        str[200], s[3][100];
    bool        reading_apps = false;

    static long layout_file_mtime;

    // if layout file doesn't exist then return 0 apps
    rc = stat(layout_file_path, &statbuf);
    if (rc != 0) {
        for (i = 0; i < max_apps; i++) {
            free(apps[i]);
            apps[i] = NULL;
        }
        max_apps = 0;
        return;
    }

    // if layout file has not changed then 
    // return without updating the list of apps
    if (statbuf.st_mtime == layout_file_mtime) {
        return;
    }
    layout_file_mtime = statbuf.st_mtime;

    // obtain the list of apps from the layout file ...

    // free the current apps names
    for (i = 0; i < max_apps; i++) {
        free(apps[i]);
        apps[i] = NULL;
    }
    max_apps = 0;

    // read the app names, which are the same as their dir names, from the layout file
    fp = fopen(layout_file_path, "r");
    while (fgets(str, sizeof(str), fp)) {
        // ignore lines that are blank or begin with comment char
        if (str[0] == '\n' || str[0] == '#') {
            continue;
        }

        // xxx
        if (strncmp(str, "APPS", 3) == 0) {
            reading_apps = true;
            continue;
        }
        if (strncmp(str, "SERVICES", 8) == 0) {
            reading_apps = false;
            break;
        }
        if (!reading_apps) {
            continue;
        }

        // read 3 app names from each line of the layout file
        cnt = sscanf(str, "%s %s %s", s[0], s[1], s[2]);
        if (cnt != 3) {
            ERROR("invalid line '%s'\n", str);
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
// - locking
// - statics
// - comments
// - name of svcs va layout

#define SERVICE_STATE_STOPPED           0     // white
#define SERVICE_STATE_RUNNING           1     // green
#define SERVICE_STATE_STOPPING          2     // yellow
#define SERVICE_STATE_STOPPED_BY_ERROR  3     // red

#define SERVICE_STATE_STR(z) \
    ((z) == SERVICE_STATE_STOPPED           ? "STOPPED"          : \
     (z) == SERVICE_STATE_RUNNING           ? "RUNNING"          : \
     (z) == SERVICE_STATE_STOPPING          ? "STOPPING"         : \
     (z) == SERVICE_STATE_STOPPED_BY_ERROR  ? "STOPPED_BY_ERROR" : \
                                              "????")

#define MAX_SERVICES 100
typedef struct {
    char *name;
    int   state;
    bool  start_pending;
    bool  delete_pending;
} service_t;

static service_t services_tbl[MAX_SERVICES];
static char     *svcs[MAX_SERVICES];
static int       max_svcs;

char             stop_requested[MAX_SERVICES];

void run_svc(int id);
void process_new_svc_names(void);
void get_list_of_svcs(bool *new_names);
int alloc_service(char *name);
void free_service(int id);
bool is_name_in_layout(char *name);
bool is_name_in_services_tbl(char *name);

static void services(void)
{
    sdl_event_t event;
    int         id;
    bool        done = false;
    bool        new_names;

    // handle the setting display
    while (true) {
        // init display and display title line
        sdl_display_init(BG_COLOR);
        sdl_print_init(DEFAULT_FONT, COLOR_WHITE, BG_COLOR);
        sdl_render_text_xyctr(sdl_win_width/2, sdl_char_height/2, "Services");

        // xxx comment
        get_list_of_svcs(&new_names);
        if (new_names) {
            process_new_svc_names();
        }

// XXX
        // display name and controls for each service
        for (id = 0; id < MAX_SERVICES; id++) {
            //service_t *x = &services_tbl[id];
            // xxx todo
        }

        // display the control event 'X' to exit this
        sdl_register_control_events(NULL, NULL, "X", BG_COLOR, 0, 0, EVID_QUIT);

        // present the display
        sdl_display_present();

        // wait for an event, with 10 ms timeout;
        // if no event received then re-display
        sdl_get_event(TEN_MS, &event);
        if (event.event_id == -1) {
            continue;
        }

        // process the event
// XXX
        INFO("proc event_id %d\n", event.event_id);
        switch (event.event_id) {
        case EVID_QUIT:
            done = true;
            break;
        }

        if (done) {
            break;
        }
    }
}

// - - - - - - - - - process event routines  - - - - - - - - - - - - - 

void process_new_svc_names(void)
{
    int id, i;

    // loop over all defined services
    for (id = 0; id < MAX_SERVICES; id++) {
        service_t *x = &services_tbl[id];
        if (x->name == NULL) {
            continue;
        }
        if (is_name_in_layout(x->name) == false) {
            if (x->state == SERVICE_STATE_RUNNING) {
                x->delete_pending = true;
                stop_requested[id] = true;
            } else if (x->state == SERVICE_STATE_STOPPING) {
                x->delete_pending = true;
            } else {
                free_service(id);
            }
        }
    }

    // loop over all svc names from the layout file
    for (i = 0; i < MAX_SERVICES; i++) {
        char *name = svcs[i];
        if (name == NULL) {
            continue;
        }
        if (is_name_in_services_tbl(name) == false) {
            alloc_service(name);
        }
    }
}

void process_start_req(int id)
{
    service_t *x = &services_tbl[id];

    if (x->name == NULL || x->state != SERVICE_STATE_STOPPED) {
        ERROR("id=%d name=%s state=%s\n", id, x->name, SERVICE_STATE_STR(x->state));
        return;
    }
    
    x->state = SERVICE_STATE_RUNNING;
    run_svc(id);
}

void process_stop_req(int id)
{
    service_t *x = &services_tbl[id];

    if (x->name == NULL || x->state != SERVICE_STATE_STOPPED) {
        ERROR("id=%d name=%s state=%s\n", id, x->name, SERVICE_STATE_STR(x->state));
        return;
    }

    stop_requested[id] = true;
}

void process_restart_req(int id)
{
    service_t *x = &services_tbl[id];

    if (x->name == NULL || x->state != SERVICE_STATE_RUNNING) {
        ERROR("id=%d name=%s state=%s\n", id, x->name, SERVICE_STATE_STR(x->state));
        return;
    }

    x->start_pending = true;
    stop_requested[id] = true;
}

void process_stopped_callback(int id, int rc)
{
    service_t *x = &services_tbl[id];

    if (x->name == NULL || x->state != SERVICE_STATE_STOPPING) {
        ERROR("id=%d name=%s state=%s\n", id, x->name, SERVICE_STATE_STR(x->state));
        return;
    }

    x->state = (rc == 0 ? SERVICE_STATE_STOPPED : SERVICE_STATE_STOPPED_BY_ERROR);

    if (x->delete_pending) {
        x->delete_pending = false;
        free_service(id);
    } else if (x->start_pending) {
        x->start_pending = false;
        x->state = SERVICE_STATE_RUNNING;
        run_svc(id);
    }
}   

// - - - - - - - - - run the svc - - - - - - - - - - - - - - 

int service_thread(void *cx);

void run_svc(int id)
{
    sdl_create_detached_thread(service_thread, (void*)(long)id);
}

int service_thread(void *cx)
{
    int id = (int)(long)cx;
    service_t *x = &services_tbl[id];
    int rc;

    rc = run_app(x->name, id);

    process_stopped_callback(id, rc);

    return 0;
}

// - - - - - - - - - get list of svcs from layout file - - - - - - - - - - 

void get_list_of_svcs(bool *new_names)
{
    const char *layout_file_path = "apps/layout";
    struct stat statbuf;
    int         rc, i, cnt, linenum=0;
    FILE       *fp;
    char        str[200], svc_name[100];
    bool        reading_svcs = false;

    static long layout_file_mtime;

    // if layout file doesn't exist then return 0 svcs
    rc = stat(layout_file_path, &statbuf);
    if (rc != 0) {
        for (i = 0; i < max_svcs; i++) {
            free(svcs[i]);
            svcs[i] = NULL;
        }
        *new_names = (max_svcs > 0);
        max_svcs = 0;
        return;
    }

    // if layout file has not changed then 
    // return without updating the list of svcs
    if (statbuf.st_mtime == layout_file_mtime) {
        *new_names = false;
        return;
    }
    layout_file_mtime = statbuf.st_mtime;

    // obtain the list of svcs from the layout file ...

    // free the current svcs names
    for (i = 0; i < max_svcs; i++) {
        free(svcs[i]);
        svcs[i] = NULL;
    }
    max_svcs = 0;

    // read the svcs names, which are the same as their dir names, from the layout file
    fp = fopen(layout_file_path, "r");
    while (fgets(str, sizeof(str), fp)) {
        linenum++;

        // ignore lines that are blank or begin with comment char
        if (str[0] == '\n' || str[0] == '#') {
            continue;
        }

        // xxx
        if (strncmp(str, "SERVICES", 8) == 0) {
            reading_svcs = true;
            continue;
        }
        if (!reading_svcs) {
            continue;
        }

        // read 1 svc names from each line of the layout file
        cnt = sscanf(str, "%s", svc_name);
        if (cnt != 1) {
            ERROR("invalid line %d in layout file\n", linenum);
            break;
        }

        // store the svc names, just read, to the svcs[] array;
        svcs[max_svcs++] = strdup(svc_name);
    }
    fclose(fp);

    // debug print the list of svcs
    INFO("max_svcs = %d\n", max_svcs);
    for (i = 0; i < max_svcs; i++) {
        INFO("svcs[%d] = %s\n", i, svcs[i]);
    }

    // service names may have changed
    *new_names = true;
    return;
}

// - - - - - - - - - misc support routines - - - - - - - - - - 

int alloc_service(char *name)
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
    INFO("allocated %d\n", id);

    service_t *x = &services_tbl[id];
    memset(x, 0, sizeof(service_t));
    x->name = strdup(name);

    return id;
}

void free_service(int id)
{
    service_t *x = &services_tbl[id];

    free(x->name);
    x->name = NULL;
    memset(x, 0, sizeof(service_t));
}

bool is_name_in_layout(char *name)  // xxx name of this routine ?
{
    for (int i = 0; i < max_svcs; i++) {
        if (strcmp(svcs[i], name) == 0) {
            return true;
        }
    }

    return false;
}

bool is_name_in_services_tbl(char *name)
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
    sdl_event_t event;
    sdl_loc_t  *loc;
    bool        done = false;
    char       *msg = NULL;
    long        msg_time = 0;
    char       *ipaddr;

    #define EVID_DEVEL_MODE         1000
    #define EVID_DEVEL_PORT         1001
    #define EVID_RESET_APPS         1002
    #define EVID_COPYRIGHT          1004
    #define EVID_STOP_REQUESTED     1006 //xxx del

    // get this device ipaddr
    ipaddr = util_get_ipaddr();
    INFO("SETTINGS %s:%d\n", ipaddr, params.devel_port);

    // handle the setting display
    while (true) {
        // init display and display title line
        sdl_display_init(BG_COLOR);
        sdl_print_init(DEFAULT_FONT, COLOR_WHITE, BG_COLOR);
        sdl_render_text_xyctr(sdl_win_width/2, sdl_char_height/2, "Settings");

        // display version
        sdl_render_printf(0, ROW2Y(2), "Version = %s", VERSION);
        sdl_render_printf(0, ROW2Y(3), "%s", BUILD_DATE);

        // init print color to COLOR_LIGHT_BLUE for the following,
        // because these all are selectable
        sdl_print_init_color(COLOR_LIGHT_BLUE, BG_COLOR);

        // display Copyright
        loc = sdl_render_printf(0, ROW2Y(5), "Copyright");
        sdl_register_event(loc, EVID_COPYRIGHT);

        // display Devel_Mode
        loc = sdl_render_printf(0, ROW2Y(7), "Devel_Mode = %s", params.devel_mode ? "ON" : "OFF");
        sdl_register_event(loc, EVID_DEVEL_MODE);

        // display Devel_Port
        loc = sdl_render_printf(0, ROW2Y(9), "Devel_Port = %d", params.devel_port);
        sdl_register_event(loc, EVID_DEVEL_PORT);

        // display Reset_Apps
        loc = sdl_render_printf(0, ROW2Y(11), "Reset_Apps");
        sdl_register_event(loc, EVID_RESET_APPS);

        // display t1 xxx unit test
        loc = sdl_render_printf(0, ROW2Y(13), "stop_requested = %d", stop_requested[5]);
        sdl_register_event(loc, EVID_STOP_REQUESTED);

        // change print color back to white
        sdl_print_init_color(COLOR_WHITE, BG_COLOR);

        // if a message is requested for display then do so;
        // otherwise, when in developer mode, display ipaddr:port
        if (msg && (util_microsec_timer() - msg_time) < 3000000) {
            sdl_render_printf(0, sdl_win_height-400, "%s", msg);
        } else if (params.devel_mode) {
            sdl_render_printf(0, sdl_win_height-400, "%s:%d", ipaddr, params.devel_port);
        }

        // display the control event 'X' to exit this screen
        sdl_register_control_events(NULL, NULL, "X", BG_COLOR, 0, 0, EVID_QUIT);

        // present the display
        sdl_display_present();

        // wait for an event, with 10 ms timeout;
        // if no event received then re-display
        sdl_get_event(TEN_MS, &event);
        if (event.event_id == -1) {
            continue;
        }

        // process the event
        INFO("proc event_id %d\n", event.event_id);
        switch (event.event_id) {
        case EVID_DEVEL_MODE:
            params.devel_mode = (params.devel_mode ? 0 : 1);
            util_set_int_param(".", "devel_mode", params.devel_mode);
            if (!params.devel_mode) {
                INFO("sending SIGUSR2 to server_thread\n");
                //xxx pthread_kill(server_tid, SIGUSR2);
            }
            break;
        case EVID_DEVEL_PORT: {
            char *str; 
            int cnt, port;
            str = sdl_get_input_str("Port?", true, BG_COLOR);
            cnt = sscanf(str, "%d", &port);
            if (cnt == 1 && (port >= 1024 && port <= 49151)) {
                params.devel_port = port;
                util_set_int_param(".", "devel_port", port);
                if (params.devel_mode) {
                    INFO("sending SIGUSR2 to server_thread\n");
                    //xxx pthread_kill(server_tid, SIGUSR2);
                }
            }
            break; }
        case EVID_RESET_APPS: {
            char *str; 
            str = sdl_get_input_str("Reset Apps y/n?", false, BG_COLOR);
            if (strcasecmp(str, "y") == 0) {
                create_default_apps();
                msg = "Apps are reset.";
                msg_time = util_microsec_timer();
            }
            break; }
        case EVID_COPYRIGHT:
            copyright();
            break;
//      case EVID_STOP_REQUESTED:  xxx 
//          stop_requested[5] = true;
//          break;
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
    sdl_event_t event;
    bool        done = false;

    // read the copyright file
    str = util_read_file(".", "copyright", &len);
    if (str == NULL) {
        ERROR("failed to read copyright file\n");
        return;
    }

    // init vars
    y_display_begin = 100;
    y_display_end = sdl_win_height - 200;
    y_top = y_display_begin;

    // display copyright, support motion (for scrolling)
    while (true) {
        // display copyright and register for motion (scrolling) & exit events
        sdl_display_init(BG_COLOR);
        sdl_print_init(SMALLEST_FONT, COLOR_WHITE, BG_COLOR);
        sdl_register_event(NULL, EVID_MOTION);
        sdl_render_multiline_text(y_top, y_display_begin, y_display_end, str);
        sdl_register_control_events(NULL, NULL, "X", BG_COLOR, 0, 0, EVID_QUIT);
        sdl_display_present();

        sdl_get_event(-1, &event);
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
        sdl_create_detached_thread(process_req_thread, (void*)(long)sockfd);
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
