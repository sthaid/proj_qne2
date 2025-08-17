#include <std_hdrs.h>
#include <signal.h>  //xxx add to std_hdrs

#include <sdl.h>   // xxx does this bring in SDL.h?
#include <utils.h>
#include <logging.h>

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

#define DEFAULT_FONTSZ  20
#define LARGE_FONTSZ    10

#define DEFAULT_DEVEL_PORT 9000   // IANA registered port range 1024 - 49151

#define TEN_MS 10000
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

static const char *storage_path;
static params_t    params;
static pthread_t   server_tid;

//
// prototypes 
//

static void controller(void);
static void *server_thread(void *cx);
//static char *sock_addr_to_str(char * s, int slen, struct sockaddr * addr);
//static void remove_trailing_newline(char *s);  xxx move to utils

//
// routines to launch a C program using picoc interpreter
//

int picoc_fg(char *args);
void picoc_bg(char *args);

// -----------------  MAIN  ------------------------------------------

static void init(void);
static void create_default_apps(void);

int MAIN(int argc, char **argv)
{
    // initialize
    init();

    // xxx comment
    controller();

    // end program
    INFO("TERMINATING\n");
    return 0;
}

static void sigusr1_hndlr(int signum)
{
    // nothing needed here
}

static void init(void)
{
    int rc;
    struct stat statbuf;
    char log_path[100];

    // determine storage_path, and 
    // set current working directory to storage_path
#ifdef ANDROID
    storage_path = SDL_GetAndroidInternalStoragePath();
#else
    storage_path = "/home/haid/proj/proj_qne2/linux/files";
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

    // xxx temporary
    params.devel_mode = 1;

    // if apps dir doesn't exist then create it
    rc = stat("apps", &statbuf);
    if (true || rc != 0 || !S_ISDIR(statbuf.st_mode)) {  //xxx true, del true
        create_default_apps();
    }

    // xxx comment
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = sigusr1_hndlr;
    sigaction(SIGUSR2, &action, NULL);

    // create server thread
    pthread_create(&server_tid, NULL, server_thread, NULL);
}

static void create_default_apps(void)
{
    INFO("creating default apps\n");
#ifndef ANDROID
    system("rm -rf apps");
    system("tar -xvf ../assets/apps.tar");
#else
    void *ptr;
    int rc;
    size_t len;

    ptr = SDL_LoadFile("apps.tar", &len);
    if (ptr == NULL ) {
        ERROR("failed to read apps.tar");
        return;
    }

    rc = util_write_file("tmp_apps.tar", ptr, len);  // xxx why tmp_apps.tar name?
    SDL_free(ptr);
    if (rc != 0) {
        ERROR("failed to write tmp_apps.tar\n");
        return;
    }

    rc = system("rm -rf apps");
    if (rc != 0) {
        ERROR("rm -rf apps, failed\n");
    }

    rc = system("tar -xvf tmp_apps.tar");  // xxx would just the tar work
    if (rc != 0) {
        ERROR("tar -xvf tmp_apps.tar, failed\n");
    }

    rc = unlink("tmp_apps.tar");
    if (rc != 0) {
        ERROR("failed to unlink tmp_apps.tar, %s\n", strerror(errno));
    }
#endif
}

// -----------------  CONTROLLER  ------------------------------------

#define MAX_APPS 100

static char *apps[MAX_APPS];
static int   max_apps;
static int   page;
static int   last_page;

static void display_menu(void);
static void get_list_of_apps(void);
static void settings(void);

static void controller(void)
{
    sdl_event_t event;

    // xxx should this be in init()
    sdl_init(); //xxx handle ret
    INFO("sdl_win_width,height = %d %d  sdl_char_width,height=%d %d\n",
        sdl_win_width, sdl_win_height, sdl_char_width, sdl_char_height);

    while (true) {
        // xxx reset other stuff here too, fontsz, color
        sdl_display_init(BG_COLOR);

        // xxx comment
        sdl_print_init(DEFAULT_FONTSZ, COLOR_WHITE, BG_COLOR);

        // display menu, and register for sdl events
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
                page = last_page;
            }
        } else if (event.event_id == EVID_PAGE_INCREMENT) {
            if (++page > last_page) {
                page = 0;
            }
        } else if (event.event_id == max_apps-1) {
            INFO("running Settings\n");
            settings();
            INFO("done Settings\n");
        } else if (event.event_id >= 0 && event.event_id < max_apps-1) {
            char           app_dir[100], picoc_args[1000];
            int            id = event.event_id;
            int            rc;
            DIR           *dir;
            struct dirent *dirent;

            INFO("running %s\n", apps[id]);

            sdl_print_init(DEFAULT_FONTSZ, COLOR_WHITE, COLOR_BLACK);
            sprintf(app_dir, "apps/%s", apps[id]);
            chdir(app_dir);

            // construct list of *.c files in this dir
            picoc_args[0] = '\0';
            dir = opendir(".");
            while ((dirent = readdir(dir)) != NULL) {
                char *fn = dirent->d_name;
                if (strstr(fn, ".c")) {
                    strcat(picoc_args, fn);
                    strcat(picoc_args, " ");
                }
            }
            closedir(dir);

            INFO("XXX picoc_args = %s\n", picoc_args);
            rc = picoc_fg(picoc_args);  // args

            chdir(storage_path);

            INFO("done %s, rc=%d\n", apps[id], rc);
            // xxx if app fails, put up screen with error message
        }
    }

    // xxx should this move
    sdl_exit();
}

static void display_menu(void)
{
    static sdl_texture_t *circle;

    #define RADIUS 100

    // xxx
    if (circle == NULL) {
        circle = sdl_create_filled_circle_texture(RADIUS, COLOR_BLUE);
    }

    // xxx
    get_list_of_apps();

    // xxx comment, xxx fix for multi page
    for (int id = 0; id < max_apps; id++) {
        //char *name = menu[page][id].name;
        char str1[32], str2[32];
        int len1, len2, len_max, x, y;
        //double numchars, chw, chh;
        sdl_loc_t loc;
        double chw, chh;
        int numchars;
        

#if 0
        // if name contains '_' then divide name to 2 strings,
        // else one string
        memset(str1, 0, sizeof(str1));
        memset(str2, 0, sizeof(str2));
        if ((p = strchr(name, '_')) != NULL) {
            memcpy(str1, name, p-name);
            strcpy(str2, p+1);
        } else {
            strcpy(str1, name);
        }
#else
//xxx  IMPROVE THE icons
        strcpy(str1, apps[id]);
        str2[0] = '\0';
#endif

        len1 = strlen(str1);
        len2 = strlen(str2);
        len_max = (len1 > len2 ? len1 : len2);

        // determine the size of the chars that appear in the menu item;
        // the size is determined differently if there are 2 strings vs 1;
        // the numeric values were determined experimentally
        if (len2 == 0) {
            if (len_max == 1) {
                chw = (1.0 * RADIUS) / len_max;
            } else {
                chw = (1.5 * RADIUS) / len_max;
            }
        } else {
            if (len_max == 1) {
                chw = (0.59 * RADIUS) / len_max;
            } else if (len_max == 2) {
                chw = (1.0 * RADIUS) / len_max;
            } else if (len_max == 3) {
                chw = (1.4 * RADIUS) / len_max;
            } else {
                chw = (1.5 * RADIUS) / len_max;
            }
        }
        chh = chw / 0.6;
        numchars = sdl_win_width / chw;

        // determine dispaly location of the center of the menu item
        x = (sdl_win_width/3/2) + (id%3) * (sdl_win_width/3);
        y = (sdl_win_height/6/2) + (id/3) * (sdl_win_height/6);

        // display the menu item
        sdl_render_texture(x-RADIUS, y-RADIUS, -1, -1,  0, circle);

        sdl_print_init(numchars, COLOR_WHITE, COLOR_BLUE);
        if (len2 == 0) {
            sdl_render_text_xyctr(x, y, str1);
        } else {
            sdl_render_text_xyctr(x, rint(y-0.5*chh), str1);
            sdl_render_text_xyctr(x, rint(y+0.5*chh), str2);
        }

        // register event
        loc.x = x - RADIUS;
        loc.y = y - RADIUS;
        loc.w = 2 * RADIUS;
        loc.h = 2 * RADIUS;
        sdl_register_event(&loc, id);
    }

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
    if (last_page > 0) {
        DISPLAY_CONTROL_ITEM(0,"<",EVID_PAGE_DECREMENT);
        DISPLAY_CONTROL_ITEM(1,">",EVID_PAGE_INCREMENT);
    }
    DISPLAY_CONTROL_ITEM(2,"X",EVID_QUIT);

    sdl_print_init(DEFAULT_FONTSZ, COLOR_WHITE, BG_COLOR);
}

static int qsort_compare(const void *a, const void *b)
{
    const char *str_a = *(char **)a;
    const char *str_b = *(char **)b;
    int rc;

    rc = strcmp(str_a, str_b);
    printf("%s  %s  %d\n", str_a, str_b, rc);
    return rc;
}

// xxx
static void get_list_of_apps(void)
{
    struct stat statbuf;
    char        apps_dir_path[100];
    DIR        *apps_dir;
    struct dirent *dirent;
    int         i, rc;

    static long apps_dir_mtime;

    // if apps dir has not changed then return
    sprintf(apps_dir_path, "%s/apps", storage_path);
    rc = stat(apps_dir_path, &statbuf);
    if (rc != 0) {
        ERROR("stat %s failed, %s\n", apps_dir_path, strerror(errno));
        return;
    }
    if (statbuf.st_mtime == apps_dir_mtime) {
        return;
    }
    apps_dir_mtime = statbuf.st_mtime;

    // free the current apps names
    for (i = 0; i < max_apps; i++) {
        free(apps[i]);
        apps[i] = NULL;
    }
    max_apps = 0;

    // obtain apps directory content,
    // these are the names of the defined apps
    apps_dir = opendir(apps_dir_path);
    while ((dirent = readdir(apps_dir)) != NULL) {
        if (dirent->d_name[0] == '.') {
            continue;
        }
        apps[max_apps++] = strdup(dirent->d_name);
    }
    closedir(apps_dir);

    // sort list alphabetical
    qsort(apps, max_apps, sizeof(char*), qsort_compare);

    // add settings to the end
    apps[max_apps++] = strdup("settings");

    // debug print the list of apps names
    for (i = 0; i < max_apps; i++) {
        INFO("apps[%d] = %s\n", i, apps[i]);
    }
}

#define ROW2Y(r) ((r) * sdl_char_height)  // xxx ctr vs ...
#define ROW2Y_CTR(r) ((r) * sdl_char_height + sdl_char_height/2)
#define NK2X(n,k) ((sdl_win_width/2/(n)) + (k) * (sdl_win_width/(n)))

static void settings(void)
{
    sdl_event_t event;
    sdl_loc_t  *loc;
    bool        quit = false;
    int         size;
    char       *msg = NULL;
    long        msg_time = 0;

    INFO("SETTINGS\n");

    #define EVID_DEVEL_MODE         1000
    #define EVID_DEVEL_PORT         1001
    #define EVID_RESET_APPS         1002
    #define EVID_LOG_FILE_CLEAR     1003

    while (true) {
        sdl_display_init(BG_COLOR);
        sdl_print_init(DEFAULT_FONTSZ, COLOR_WHITE, BG_COLOR);
        sdl_render_text_xyctr(sdl_win_width/2, sdl_char_height/2, "Settings");

        sdl_render_printf(0, ROW2Y(2), "Version = %s", VERSION);

        sdl_render_printf(0, ROW2Y(4), "Copyright");

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
            loc = sdl_render_printf(0, ROW2Y(12), "Log_Clear sz=%d", size);
        } else {
            loc = sdl_render_printf(0, ROW2Y(12), "Log_Clear sz=%d M", size/1000000);
        }
        sdl_register_event(loc, EVID_LOG_FILE_CLEAR);
        sdl_print_init(-1, COLOR_WHITE, BG_COLOR);

        if (msg && (util_microsec_timer() - msg_time) < 3000000) {
            sdl_render_printf(0, sdl_win_height-300, "%s", msg);
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
                pthread_kill(server_tid, SIGUSR2);  // xxx not working on android
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
                INFO("sending SIGUSR2 to server_thread\n");
                pthread_kill(server_tid, SIGUSR2);
            }
            break; }
        case EVID_RESET_APPS: {
            char *str; 
            str = sdl_get_input_str("Reset Apps y/n?", false, BG_COLOR);
            INFO("GOT STR '%s'\n", str);
            if (strcasecmp(str, "y") == 0) {
                INFO("XXX resetting apps\n");
                create_default_apps();
                msg = "Apps are reset.";
                msg_time = util_microsec_timer();
            }
            // xxx display msg for 2 secs
            break; }
        case EVID_LOG_FILE_CLEAR:   
            log_clear();
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

// ----------------- SERVER ----------------------------

static void *process_req_thread(void *cx);
static void process_req_using_android_sh(int sockfd, char *cmd);

static void *server_thread(void *cx)
{
    struct sockaddr_in server_address;
    int                listen_sockfd, ret;
    pthread_t          tid;

again:
    // wait for developer mode to be enabled
    sleep(1);
    while (params.devel_mode == false) {
        printf("xxx waiting for devl mode0\n");
        sleep(1);
    }

    INFO("SERVER_THREAD STARTING, listening on port %d\n", params.devel_port);

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
        //char               peer_addr_str[200];

        // accept connection
        peer_addr_len = sizeof(peer_addr);
        sockfd = accept(listen_sockfd, (struct sockaddr *) &peer_addr, &peer_addr_len);
        if (sockfd == -1) {
            ERROR("accept, %s\n", strerror(errno));
            break;
        }
        //sock_addr_to_str(peer_addr_str, sizeof(peer_addr_str), (struct sockaddr *)&peer_addr);
        //INFO("accepted connection from %s, sockfd=%d\n", peer_addr_str, sockfd);

        // create thread to process the client request
        pthread_create(&tid, NULL, process_req_thread, (void*)(long)sockfd);
    }

    // close listen socket,
    // goto top to wait for developer mode enabled
    close(listen_sockfd);
    goto again;

    // not reached
    INFO("SERVER_THREAD TERMINATING\n");
    return NULL;
}

static void *process_req_thread(void *cx)
{
    int sockfd = (int)(long)cx;

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
    //INFO("cmd '%s'\n", cmd);

    // xxx comment
    if (fork() == 0) {
        process_req_using_android_sh(sockfd, cmd);
    }

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

// -----------------  UTILS  ----------------------------------

#if 0
static char * sock_addr_to_str(char * s, int slen, struct sockaddr * addr)
{
    char addr_str[100];
    int port2;

    if (addr->sa_family == AF_INET) {
        inet_ntop(AF_INET,
                  &((struct sockaddr_in*)addr)->sin_addr,
                  addr_str, sizeof(addr_str));
        port2 = ((struct sockaddr_in*)addr)->sin_port;
#if 0 //xxx
    } else if (addr->sa_family == AF_INET6) {
        inet_ntop(AF_INET6,
                  &((struct sockaddr_in6*)addr)->sin6_addr,
                 addr_str, sizeof(addr_str));
        port2 = ((struct sockaddr_in6*)addr)->sin6_port;
#endif
    } else {
        snprintf(s,slen,"Invalid AddrFamily %d", addr->sa_family);
        return s;
    }

    snprintf(s,slen,"%s:%d",addr_str,ntohs(port2));
    return s;
}
#endif

#if 0
static void remove_trailing_newline(char *s)
{
    int len = strlen(s);

    if (len > 0) {
        s[len-1] = '\0';
    }
}
#endif
