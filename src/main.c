#include <std_hdrs.h>

#include <sdl.h>
#include <utils.h>
#include <logging.h>

#ifdef ANDROID
#include <SDL3/SDL.h>
#endif

//
// defines
//

#define VERSION "0.0"

#ifdef ANDROID
#define MAIN SDL_main
#else
#define MAIN main
#endif

#define EVID_PAGE_DECREMENT 1000
#define EVID_PAGE_INCREMENT 1001

#define BG_COLOR (!params.devel_mode ? COLOR_TEAL : COLOR_VIOLET)

#define SMALL_FONTSZ    30
#define DEFAULT_FONTSZ  20
#define LARGE_FONTSZ    10

#define DEFAULT_DEVEL_PORT 9000   // IANA registered port range 1024 - 49151

#define TEN_MS 10000
#define ONE_SEC 1000000

#define LAST_PAGE ((max_apps - 1) / 18)

#define DEVEL

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
static pthread_t   waiter_tid;

//
// prototypes 
//

static void processing(void);
static void *server_thread(void *cx);
static void *waiter_thread(void *cx);
static void kill_child_processes(pid_t pid);

//
// routines to launch a C program using picoc interpreter
//

int picoc_fg(char *args);
void picoc_bg(char *args);

// -----------------  MAIN  ------------------------------------------

static void init(void);
static void cleanup(void);
static void create_default_apps(void);
static void copy_asset_file(char *asset_filename, char *dest_dir);
static void sigusr1_hndlr(int signum);

int MAIN(int argc, char **argv)
{
    init();
    processing();
    cleanup();
    return 0;
}

static void init(void)
{
    char log_path[100];

    // determine storage_path, and 
    // set current working directory to storage_path
#ifdef ANDROID
    storage_path = (char*)SDL_GetAndroidInternalStoragePath();
#else
    static char storage_path_buff[200];
    getcwd(storage_path_buff, sizeof(storage_path_buff));
    strcat(storage_path_buff, "/files");
    storage_path = storage_path_buff;
#endif
    chdir(storage_path);

    // init logging
    sprintf(log_path, "%s/%s", storage_path, "log");
    log_init(log_path);

    // print startup messages
    INFO("========== STARTING: VERSION=%s ==========\n", VERSION);
    INFO("storage_path = %s\n", storage_path);

    // get params, if they don't exist, set to default value
    params.devel_mode = util_get_int_param("devel_mode", 0);
    params.devel_port = util_get_int_param("devel_port", DEFAULT_DEVEL_PORT);

    // copy asset files to storage_path
    copy_asset_file("apps.tar", storage_path);
    copy_asset_file("copyright", storage_path);
    copy_asset_file("FreeMonoBold.ttf", storage_path);

#ifdef DEVEL
    // for development:
    // - enable developer mode
    // - init apps to default
    params.devel_mode = 1;
    util_set_int_param("devel_mode", 1);
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
    pthread_create(&server_tid, NULL, server_thread, NULL);
    pthread_create(&waiter_tid, NULL, waiter_thread, NULL);

    // init sdl
    sdl_init();
    INFO("sdl_win_width,height = %d %d  sdl_char_width,height=%d %d\n",
         sdl_win_width, sdl_win_height, sdl_char_width, sdl_char_height);
}

static void cleanup(void)
{
    sdl_exit();

    kill_child_processes(getpid());

    INFO("TERMINATING\n");
}

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

static void copy_asset_file(char *asset_filename, char *dest_dir)
{
    int rc;
    char dest_path[200];

    sprintf(dest_path, "%s/%s", dest_dir, asset_filename);

#ifndef ANDROID
    char cmd[250];

    sprintf(cmd, "cp %s/../assets/%s %s", storage_path, asset_filename, dest_path);
    rc = system(cmd);
    if (rc != 0) {
        ERROR("cmd '%s' failed\n", cmd);
    }
#else
    void  *ptr;
    size_t len;

    // remove dest file, in case it already exists
    unlink(dest_path);

    // read the asset using SDL_LoadFile;
    //
    // Note SDL_LoadFile calls SDL_IOFromFile, which attempts to read
    // the file as follows:
    // - if filename begins with '/' then use fopen
    //   else if filename begins with "content://" then use Android_JNI_OpenFileDescriptor
    //   else fopen of file in SDL_GetAndroidInternalStoragePath
    //   endif
    // - if above failed then try to read the file from assets, using Android_JNI_FileOpen
    ptr = SDL_LoadFile(asset_filename, &len);
    if (ptr == NULL ) {
        ERROR("failed to read apps.tar");
        return;
    }

    // write the file to dest_path
    rc = util_write_file(dest_path, ptr, len);
    SDL_free(ptr);
    if (rc != 0) {
        ERROR("failed to write %s\n", dest_path);
        return;
    }
#endif
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

static void display_menu(void);
static void get_list_of_apps(void);
static void settings(void);

static void processing(void)
{
    sdl_event_t event;

    while (true) {
        // clear the display, and set the font to default
        sdl_display_init(BG_COLOR);
        sdl_print_init(DEFAULT_FONTSZ, COLOR_WHITE, BG_COLOR);

        // display menu, and register for events
        display_menu();

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
        } else if (event.event_id == EVID_PAGE_DECREMENT) {
            if (--page < 0) {
                page = LAST_PAGE;
            }
        } else if (event.event_id == EVID_PAGE_INCREMENT) {
            if (++page > LAST_PAGE) {
                page = 0;
            }
        } else if (event.event_id >= 0 && event.event_id <= max_apps-1) {
            char           app_dir[100], picoc_args[1000];
            int            id = event.event_id;
            int            rc;
            DIR           *dir;
            struct dirent *dirent;

            if (apps[id] == NULL) {
                ERROR("apps[%d] is NULL\n", id);
            } else {
                INFO("running %s\n", apps[id]);

                sdl_print_init(DEFAULT_FONTSZ, COLOR_WHITE, COLOR_BLACK);

                if (strcmp(apps[id], "Settings") == 0) {
                    settings();
                } else {
                    sprintf(app_dir, "apps/%s", apps[id]);
                    chdir(app_dir);

                    // construct list of *.c files in this dir
                    picoc_args[0] = '\0';
                    dir = opendir(".");
                    while ((dirent = readdir(dir)) != NULL) {
                        char *fn = dirent->d_name;
                        int len = strlen(fn);
                        if (len > 2 && strcmp(fn+len-2, ".c") == 0) {
                            strcat(picoc_args, fn);
                            strcat(picoc_args, " ");
                        }
                    }
                    closedir(dir);

                    if (picoc_args[0] != '\0') {
                        INFO("picoc_args = %s\n", picoc_args);
                        rc = picoc_fg(picoc_args);  // args
                        INFO("done %s, rc=%d\n", apps[id], rc);
                    } else {
                        ERROR("no source code in %s\n", app_dir);
                    }

                    chdir(storage_path);
                }
            }
        }
    }
}

// -----------------  xxxxxxxx  -----------------------------------

static void display_menu(void)
{
    static sdl_texture_t *circle;
    int first, last;

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
        sdl_print_init(SMALL_FONTSZ, COLOR_WHITE, BG_COLOR);
        sdl_render_printf_xyctr(sdl_win_width/2, sdl_char_height/2, "Page %d", page);
        sdl_print_init(DEFAULT_FONTSZ, COLOR_WHITE, BG_COLOR);
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

// XXX
// xxx improve below
    // xxx
    sdl_print_init(LARGE_FONTSZ, COLOR_WHITE, BG_COLOR);

    // xxx move this
    // xxx apps should take advantage of this
    #define DISPLAY_CONTROL_ITEM(col,str,evid) \
        do { \
            sdl_loc_t *loc; \
            int x = (sdl_win_width/3/2) + (col) * (sdl_win_width/3); \
            int y = sdl_win_height - sdl_char_height/2; \
            loc = sdl_render_text_xyctr(x, y, str); \
            sdl_register_event(loc, evid); \
        } while (0)

    // xxx no arrows if not needed
    // xxx dont display if at begining or end
    if (LAST_PAGE > 0) {
        DISPLAY_CONTROL_ITEM(0,"<",EVID_PAGE_DECREMENT);
        DISPLAY_CONTROL_ITEM(1,">",EVID_PAGE_INCREMENT);
    }
    DISPLAY_CONTROL_ITEM(2,"X",EVID_QUIT);

    sdl_print_init(DEFAULT_FONTSZ, COLOR_WHITE, BG_COLOR);
}

// -----------------  xxxxxxxx  -----------------------------------

// xxx explain this
static void get_list_of_apps_from_layout_file(char *layout_file_path);
static void get_list_of_apps_from_apps_dirs(char *apps_dir_path);

static void get_list_of_apps(void)
{
    char layout_file_path[100];
    char apps_dir_path[100];
    int rc;
    struct stat statbuf;

    static long layout_file_mtime;
    static long apps_dir_mtime;

    // if layout file exists
    //   if layout file has changed then
    //     get_list_of_apps_from_layout_file
    //   endif
    // endif
    sprintf(layout_file_path, "%s/apps/layout", storage_path);
    rc = stat(layout_file_path, &statbuf);
    if (rc == 0) {
        if (statbuf.st_mtime != layout_file_mtime) {
            get_list_of_apps_from_layout_file(layout_file_path);
            layout_file_mtime = statbuf.st_mtime;
        }
        apps_dir_mtime = 0;
        return;
    }

    // if apps dir exists
    //   if apps dir has changed then
    //     get_list_of_apps_from_apps_dirs
    //   endif
    // endif
    sprintf(apps_dir_path, "%s/apps", storage_path);
    rc = stat(apps_dir_path, &statbuf);
    if (rc == 0) {
        if (statbuf.st_mtime != apps_dir_mtime) {
            get_list_of_apps_from_apps_dirs(apps_dir_path);
            apps_dir_mtime = statbuf.st_mtime;
        }
        layout_file_mtime = 0;
        return;
    }

    ERROR("failed to stat %s, %s\n", apps_dir_path, strerror(errno));
    apps_dir_mtime = 0;
    layout_file_mtime = 0;
    return;
}

static void get_list_of_apps_from_layout_file(char *layout_file_path)
{
    char str[200], s[3][50];
    int i, cnt;
    FILE *fp;

    // free the current apps names
    for (i = 0; i < max_apps; i++) {
        free(apps[i]);
        apps[i] = NULL;
    }
    max_apps = 0;

    // read the app names, which are the same as their dir names,
    // from the layout file
    fp = fopen(layout_file_path, "r");
    while (fgets(str, sizeof(str), fp)) {
        // ignore lines that are blank or begin with comment char
        if (str[0] == '\n' || str[0] == '#') {
            continue;
        }

        // read 3 app names from each line of the layout file
        cnt = sscanf(str, "%s %s %s", s[0], s[1], s[2]);
        if (cnt != 3) {
            ERROR("invalid line '%s'\n", str);
            break;
        }

        // store the app names just read in the apps[] array;
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

static int qsort_compare(const void *a, const void *b)
{
    const char *str_a = *(char **)a;
    const char *str_b = *(char **)b;
    int rc;

    rc = strcmp(str_a, str_b);
    return rc;
}

static void get_list_of_apps_from_apps_dirs(char *apps_dir_path)
{
    DIR           *apps_dir;
    struct dirent *dirent;
    int            i;

    // free the current apps names
    for (i = 0; i < max_apps; i++) {
        free(apps[i]);
        apps[i] = NULL;
    }
    max_apps = 0;

    // obtain apps directory content,
    // these are the names of the apps
    apps_dir = opendir(apps_dir_path);
    while ((dirent = readdir(apps_dir)) != NULL) {
        if (dirent->d_name[0] == '.') {
            continue;
        }
        apps[max_apps++] = strdup(dirent->d_name);
    }
    closedir(apps_dir);

    // add Settings; Settings is a special 'app', 
    // it is implemented by the settings() routine in this
    // file; it is not executed by picoc as the other apps are
    apps[max_apps++] = strdup("Settings");

    // sort list alphabetical
    qsort(apps, max_apps, sizeof(char*), qsort_compare);

    // debug print the list of apps names
    INFO("max_apps = %d\n", max_apps);
    for (i = 0; i < max_apps; i++) {
        if (apps[i] != NULL) {
            INFO("apps[%d] = %s\n", i, apps[i]);
        }
    }
}

// ----------------------------------------------------------------

// XXX
// xxx include in picoc ?
#define ROW2Y(r) ((r) * sdl_char_height)  // xxx ctr vs ...
#define ROW2Y_CTR(r) ((r) * sdl_char_height + sdl_char_height/2)
#define NK2X(n,k) ((sdl_win_width/2/(n)) + (k) * (sdl_win_width/(n)))

static void copyright(void);

static void settings(void)
{
    sdl_event_t event;
    sdl_loc_t  *loc;
    bool        quit = false;
    int         size;
    char       *msg = NULL;
    long        msg_time = 0;
    char       *ipaddr;

    #define EVID_DEVEL_MODE         1000
    #define EVID_DEVEL_PORT         1001
    #define EVID_RESET_APPS         1002
    #define EVID_LOG_FILE_CLEAR     1003
    #define EVID_COPYRIGHT          1004

    ipaddr = util_get_ipaddr();
    INFO("SETTINGS %s:%d\n", ipaddr, params.devel_port);

    // xxx comments, and cleanup
    while (true) {
        sdl_display_init(BG_COLOR);
        sdl_print_init(DEFAULT_FONTSZ, COLOR_WHITE, BG_COLOR);
        sdl_render_text_xyctr(sdl_win_width/2, sdl_char_height/2, "Settings");

        // XXX
        sdl_render_printf(0, ROW2Y(2), "Version = %s", VERSION);  // xxx add this

        // XXX
        sdl_print_init(-1, COLOR_LIGHT_BLUE, BG_COLOR);
        loc = sdl_render_printf(0, ROW2Y(4), "Copyright");  // xxx add file for this, and display it
        sdl_register_event(loc, EVID_COPYRIGHT);
        sdl_print_init(-1, COLOR_WHITE, BG_COLOR);

        sdl_print_init(-1, COLOR_LIGHT_BLUE, BG_COLOR);
        loc = sdl_render_printf(0, ROW2Y(6), "Devel_Mode = %s", params.devel_mode ? "ON" : "OFF");
        sdl_register_event(loc, EVID_DEVEL_MODE);
        sdl_print_init(-1, COLOR_WHITE, BG_COLOR);

        sdl_print_init(-1, COLOR_LIGHT_BLUE, BG_COLOR);
        loc = sdl_render_printf(0, ROW2Y(8), "Devel_Port = %d", params.devel_port);
        sdl_register_event(loc, EVID_DEVEL_PORT);
        sdl_print_init(-1, COLOR_WHITE, BG_COLOR);

        sdl_print_init(-1, COLOR_LIGHT_BLUE, BG_COLOR);
        loc = sdl_render_printf(0, ROW2Y(10), "Reset_Apps");
        sdl_register_event(loc, EVID_RESET_APPS);
        sdl_print_init(-1, COLOR_WHITE, BG_COLOR);

        sdl_print_init(-1, COLOR_LIGHT_BLUE, BG_COLOR);
        size = log_size();
        if (size < 1000000) {
            loc = sdl_render_printf(0, ROW2Y(12), "Clear_Log sz=%d", size);
        } else {
            loc = sdl_render_printf(0, ROW2Y(12), "Clear_Log sz=%d M", size/1000000);
        }
        sdl_register_event(loc, EVID_LOG_FILE_CLEAR);
        sdl_print_init(-1, COLOR_WHITE, BG_COLOR);

        if (msg && (util_microsec_timer() - msg_time) < 3000000) {
            sdl_render_printf(0, sdl_win_height-400, "%s", msg);
        } else if (params.devel_mode) {
            sdl_render_printf(0, sdl_win_height-400, "%s:%d", ipaddr, params.devel_port);
        }

        sdl_print_init(LARGE_FONTSZ, COLOR_WHITE, BG_COLOR);
        DISPLAY_CONTROL_ITEM(2,"X",EVID_QUIT);
        sdl_print_init(DEFAULT_FONTSZ, COLOR_WHITE, BG_COLOR);

        sdl_display_present();

        sdl_get_event(TEN_MS, &event);
        if (event.event_id == -1) {
            continue;
        }

        // process the event
        INFO("proc event_id %d\n", event.event_id);
        switch (event.event_id) {
        case EVID_DEVEL_MODE:
            params.devel_mode = (params.devel_mode ? 0 : 1);
            util_set_int_param("devel_mode", params.devel_mode);
            if (!params.devel_mode) {
                INFO("sending SIGUSR2 to server_thread\n");
                pthread_kill(server_tid, SIGUSR2);
            }
            break;
        case EVID_DEVEL_PORT: {
            char *str; 
            int cnt, port;
            str = sdl_get_input_str("Port?", true, BG_COLOR);
            INFO("GOT STR '%s'\n", str);
            cnt = sscanf(str, "%d", &port);
            if (cnt == 1 && (port >= 1024 && port <= 49151)) {
                params.devel_port = port;
                util_set_int_param("devel_port", port);
                if (params.devel_mode) {
                    INFO("sending SIGUSR2 to server_thread\n");
                    pthread_kill(server_tid, SIGUSR2);
                }
            }
            break; }
        case EVID_RESET_APPS: {
            char *str; 
            str = sdl_get_input_str("Reset Apps y/n?", false, BG_COLOR);
            INFO("GOT STR '%s'\n", str);
            if (strcasecmp(str, "y") == 0) {
                INFO("resetting apps\n");
                create_default_apps();
                msg = "Apps are reset.";
                msg_time = util_microsec_timer();
            }
            break; }
        case EVID_LOG_FILE_CLEAR:   
            log_clear();
            msg = "Log file is cleared.";
            msg_time = util_microsec_timer();
            break;
        case EVID_COPYRIGHT:
            copyright();
            break;
        case EVID_QUIT:
            quit = true;
            break;
        }

        if (quit) {
            break;
        }
    }
}

void copyright(void)
{
    char       *str;
    int         y_display_begin, y_display_end, y_top;
    int         len;
    sdl_event_t event;
    bool        quit = false;

    str = util_read_file("copyright", &len);
    if (str == NULL) {
        ERROR("failed to read copyright file\n");
        return;
    }

    y_display_begin = 100;
    y_display_end = sdl_win_height - 200;
    y_top = y_display_begin;

    while (true) {
        sdl_display_init(BG_COLOR);
        sdl_print_init(40, COLOR_WHITE, BG_COLOR);
        sdl_register_event(NULL, EVID_MOTION);
        sdl_render_multiline_text(y_top, y_display_begin, y_display_end, str);
        sdl_print_init(LARGE_FONTSZ, COLOR_WHITE, BG_COLOR);
        DISPLAY_CONTROL_ITEM(2,"X",EVID_QUIT);
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
            quit = true;
            break;
        }

        if (quit) {
            break;
        }
    }

    free(str);
}

// ----------------- SERVER ----------------------------

#define MAX_PID_TBL 20

static void *process_req_thread(void *cx);
static void process_req_using_android_sh(int sockfd, char *cmd);

static void *server_thread(void *cx)
{
    struct sockaddr_in server_address;
    int                listen_sockfd, ret;
    pthread_t          tid;

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
        return NULL;
    }

    // set socket options
    int reuseaddr = 1;
    ret = setsockopt(listen_sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseaddr, sizeof(reuseaddr));
    if (ret == -1) {
        ERROR("setsockopt SO_REUSEADDR, %s\n", strerror(errno));
        return NULL;
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
        return NULL;
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
        pthread_create(&tid, NULL, process_req_thread, (void*)(long)sockfd);
    }

    // close listen socket
    close(listen_sockfd);

    // kill all child processes
    kill_child_processes(getpid());

    // goto top to reinit server_thread
    goto again;

    // not reached
    INFO("SERVER_THREAD TERMINATING\n");
    return NULL;
}

static void *process_req_thread(void *cx)
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
            return NULL;
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
    return NULL;
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

static void *waiter_thread(void *cx)
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

    return NULL;
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
