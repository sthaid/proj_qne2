#include <std_hdrs.h>

#include <sdl.h>
#include <utils.h>

//
// defines
//


#ifdef ANDROID
#define MAIN SDL_main
#else
#define MAIN main
#endif

#define VERSION 1

#define EVID_PAGE_DECREMENT 1000
#define EVID_PAGE_INCREMENT 1001

#define MENU_BG_COLOR (!settings.devel_mode ? COLOR_TEAL : COLOR_VIOLET)

//
// typedefs
//

typedef struct {
    unsigned long version;
    unsigned long devel_mode;
    unsigned long reserved[14];
} settings_t;

//
// variables
//

static const char *storage_path;
static bool        server_thread_running;
static char        log_file_pathname[100];
static settings_t  settings;

//
// prototypes 
//

static void controller(void);
static void *server_thread(void *cx);

//
// routines to launch a C program using picoc interpreter
//

int picoc_fg(char *args);
void picoc_bg(char *args);

// -----------------  MAIN  ------------------------------------------

static void init(void);
static void create_default_apps(void);
static void read_settings(void);
static void write_settings(void);
static void print_settings(void);

int MAIN(int argc, char **argv)
{
    // initialize
    init();

    // xxx comment
    controller();

    // end program xxx kill the server thread ?
    INFO("TERMINATING\n");
    return 0;
}

static void init(void)
{
    pthread_t tid;
    int rc;
    struct stat statbuf;

    // determine storage_path, and 
    // set current working directory to storage_path
#ifdef ANDROID
    storage_path = SDL_AndroidGetInternalStoragePath();
#else
    storage_path = "/home/haid/proj/proj_qne2/linux/files";  //xxx
#endif
    chdir(storage_path);

    // init logging
#ifdef ANDROID
    init_logging("log");
#else
    init_logging(NULL);
#endif

    // read settings, if file deosn't exist it will be created
    read_settings();

    // print startup messages
    INFO("========== STARTING: VERSION=%ld ==========\n", settings.version);
    print_settings();

    // if apps dir doesn't exist then create it
    rc = stat("apps", &statbuf);
    if (rc != 0 || !S_ISDIR(statbuf.st_mode)) {
        create_default_apps();
    }

    // create server thread
    pthread_create(&tid, NULL, server_thread, NULL);
//  while (server_thread_running == false) {
//      usleep(10000);
//  }
}

static void create_default_apps(void)
{
    INFO("creating default apps\n");
    system("rm -rf apps");
    system("tar -xvf ../assets/apps.tar");
}

static void read_settings(void)
{
    #define UPDATE_SETTING(which) \
        if (strcmp(name, #which) == 0) { \
            settings.which = value; \
            continue; \
        }

    FILE *fp;
    char s[100];
    int cnt;
    char name[100];
    unsigned long value;

    // open settings file
    fp = fopen("settings", "r");

    // if failed to open settings file then create default settings file
    if (fp == NULL) {
        static settings_t default_settings = { VERSION };
        settings = default_settings;
        write_settings();
        return;
    }

    // read settings
    while (fgets(s, sizeof(s), fp) != NULL) {
        remove_trailing_newline(s);

        cnt = sscanf(s, "%s %ld", name, &value);
        if (cnt != 2) {
            ERROR("invalid line in settings file '%s'\n", s);
            break;
        }

        UPDATE_SETTING(version);
        UPDATE_SETTING(devel_mode);
    }

    // close 
    fclose(fp);

}

static void write_settings(void)
{
    FILE *fp;

    fp = fopen("settings", "w");
    if (fp == NULL) {
        ERROR("failed to open 'settings' for writing, %s\n", strerror(errno));
        return;
    }

    fprintf(fp, "version    %ld\n", settings.version);
    fprintf(fp, "devel_mode %ld\n", settings.devel_mode);

    fclose(fp);
}

static void print_settings(void)
{
    #define PRINT_SETTING(which) \
        INFO("  %-16s = %ld\n", #which, settings.which)

    INFO("settings ...\n");
    PRINT_SETTING(version);
    PRINT_SETTING(devel_mode);
}

// -----------------  CONTROLLER  ------------------------------------

#define MAX_PAGE 10
#define MAX_MENU 15

typedef struct {
    char *dir;
    char *name;
    char *args;
} menu_t;

static int    page;
static int    last_page;
static menu_t menu[MAX_PAGE][MAX_MENU];

static void display_menu(void);
static void read_menu(void);
static void settings_proc(void); //xxx

static void controller(void)
{
    int event_id, rc;

    rc = sdl_init(); //xxx handle ret
    INFO("sdl_win_width,height = %d %d  sdl_char_width,height=%d %d\n",
        sdl_win_width, sdl_win_height, sdl_char_width, sdl_char_height);

    while (true) {
        // xxx reset other stuff here too, fontsz, color
        sdl_display_init(MENU_BG_COLOR);

        // display menu, and register for sdl events
        display_menu();

        // update the display
        sdl_display_present();

        // wait for an event, 1 sec timeout
        event_id = sdl_get_event(1000000);
        if (event_id == -1) {
            continue;
        }

        // process the event
        INFO("proc event_id %d\n", event_id);
        if (event_id == EVID_QUIT) {
            break;
        } else if (event_id == EVID_PAGE_DECREMENT) {
            INFO("XXX GOT PAGE LEFT XXX\n");
            if (--page < 0) {
                page = last_page;
            }
        } else if (event_id == EVID_PAGE_INCREMENT) {
            INFO("XXX GOT PAGE RIGHT XXX\n");
            if (++page > last_page) {
                page = 0;
            }
        } else if (event_id == EVID_QUIT) {
            INFO("XXX GOT QUIT XXX\n");
            break;
        } else {
            // xxx check that menu entry is defined
            int pg = event_id / MAX_MENU;
            int id = event_id % MAX_MENU;

            if (pg == 0 && id == MAX_MENU-1) {
                INFO("running Settings\n");
                settings_proc();
                INFO("done Settings\n");
            } else {
                char working_dir[100];
                INFO("running %s\n", menu[pg][id].name);
                sprintf(working_dir, "apps/%s", menu[pg][id].dir);
                chdir(working_dir);
                rc = picoc_fg(menu[pg][id].args);
                chdir("../..");
                INFO("done %s, rc=%d\n", menu[pg][id].name, rc);
            }
        }
    }

    sdl_exit();
}


static void display_menu(void)
{
    int    id;
    static sdl_texture_t *circle;

    #define RADIUS 100

    // xxx
    if (circle == NULL) {
        circle = sdl_create_filled_circle_texture(RADIUS, COLOR_BLUE);
    }

    // xxx
    read_menu();

    // xxx comment
    for (id = 0; id < MAX_MENU; id++) {
        char *name = menu[page][id].name;
        char str1[32], str2[32], *p;
        int len1, len2, len_max, x, y;
        double numchars, chw, chh;
        sdl_loc_t loc;

        // if this menu entry is not defined then continue
        if (name == NULL) {
            continue;
        }

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
        sdl_render_texture(x-RADIUS, y-RADIUS, 2*RADIUS, 2*RADIUS, 0, circle);

        sdl_print_init(numchars, COLOR_WHITE, COLOR_BLUE);
        if (len2 == 0) {
            sdl_render_text(true, x, y, str1);
        } else {
            sdl_render_text(true, x, rint(y-0.5*chh), str1);
            sdl_render_text(true, x, rint(y+0.5*chh), str2);
        }

        // register event
        loc.x = x;
        loc.y = y;
        loc.w = 2 * RADIUS;
        loc.h = 2 * RADIUS;
        sdl_register_event(&loc, page * MAX_MENU + id);
    }

    // xxx
    sdl_print_init(10, COLOR_WHITE, MENU_BG_COLOR);

    // xxx use loc returned by print
    #define DISPLAY_CONTROL_ITEM(col,str,evid) \
        do { \
            int x = (sdl_win_width/3/2) + (col) * (sdl_win_width/3); \
            int y = sdl_win_height - sdl_char_height/2; \
            sdl_loc_t loc = {x, y, 2*RADIUS, 2*RADIUS}; \
            sdl_render_text(true, x, y, str); \
            sdl_register_event(&loc, evid); \
        } while (0)

    // xxx no arrows if not needed
    if (last_page > 0) {
        DISPLAY_CONTROL_ITEM(0,"<",EVID_PAGE_DECREMENT);
        DISPLAY_CONTROL_ITEM(1,">",EVID_PAGE_INCREMENT);
    }
    DISPLAY_CONTROL_ITEM(2,"X",EVID_QUIT);

    // init print xxx needed ?  maybe do at top
    //sdl_print_init(20, COLOR_WHITE, MENU_BG_COLOR, &chw, &chh, NULL, NULL);

    // xxx display "Menu" as title
    //sdl_render_printf(true, sdl_win_width-chw/2, chh/2, "%d", page);
}

static void read_menu(void)
{
    FILE *fp;
    char s[500], name[100], dir[100], *args;
    int cnt, pg, id, n, ret;
    struct stat statbuf;

    static long menu_mtime;
    static char menu_path[100];

    // construct menu_path, if not already done so
    if (menu_path[0] == '\0') {
        sprintf(menu_path, "%s/apps/menu", storage_path);
    }

    // if menu file has not changed then return
    ret = stat(menu_path, &statbuf);
    if (ret != 0) {
        ERROR("stat %s failed, %s\n", menu_path, strerror(errno));
        return;
    }
    if (statbuf.st_mtime == menu_mtime) {
        return;
    }
    menu_mtime = statbuf.st_mtime;

    // free and clear menu
    for (pg = 0; pg < MAX_PAGE; pg++) {
        for (id = 0; id < MAX_MENU; id++) {
            free(menu[pg][id].name);
            free(menu[pg][id].dir);
            free(menu[pg][id].args);
        }
    }
    memset(menu, 0, sizeof(menu));

    // xxx
    last_page = 0;
    page = 0;

    // open menu file
    fp = fopen(menu_path, "r");
    if (fp == NULL) {
        ERROR("failed to open %s, %s\n", menu_path, strerror(errno));
        return;
    }

    // read lines from menu file, and populate menu struct
    while (fgets(s, sizeof(s), fp) != NULL) {
        remove_trailing_newline(s);

        // allow blank or comment lines
        if (s[0] == '\0' || s[0] == '#') {
            continue;
        }

        // extract: pg, id, name, and args:
        // example of line in menu file:
        //   "0 5 test_app test test.c"
        //   - pg       = menu page
        //   - id       = location on menu page
        //   - app_name = name shown on the menu page
        //   - dir      = app is in dir files/apps/<dir>
        //   - args     = args passed to picoc xxx explain more
        name[0] = '\0';
        id = n = 0;
        cnt = sscanf(s, "%d %d %s %s %n", &pg, &id, name, dir, &n);
        if (cnt < 4 || n == 0 || pg < 0 || pg >= MAX_PAGE || id < 0 || id >= MAX_MENU) {
            ERROR("invalid line in menu file, '%s'\n", s);
            continue;
        }
        args = s+n;

        // save values in the menu table
        menu[pg][id].name = strdup(name);
        menu[pg][id].dir = strdup(dir);
        menu[pg][id].args = strdup(args);

        // keep track of last menu page
        if (pg > last_page) {
            last_page = pg;
        }
    }

    // close menu file
    fclose(fp);

    // xxx
    menu[0][MAX_MENU-1].name = strdup("Settings");
    menu[0][MAX_MENU-1].dir  = NULL;
    menu[0][MAX_MENU-1].args = NULL;

    // debug print the new menu
    INFO("menu is now ...\n");
    for (pg = 0; pg < MAX_PAGE; pg++) {
        for (id = 0; id < MAX_MENU; id++) {
            if (menu[pg][id].name != NULL) {
                INFO("%2d %2d  %16s  %8s  %s\n", pg, id, menu[pg][id].name, menu[pg][id].dir, menu[pg][id].args);
            }
        }
    }
}

#define ROW2Y(r) ((r) * sdl_char_height)  // xxx ctr vs ...
#define ROW2Y_CTR(r) ((r) * sdl_char_height + sdl_char_height/2)
#define NK2X(n,k) ((sdl_win_width/2/(n)) + (k) * (sdl_win_width/(n)))

static void settings_proc(void)
{
    int event_id;
    sdl_loc_t *loc;
    bool quit = false;

    INFO("SETTINGS\n");

#define EVID_DEVEL_MODE 1000
#define EVID_RESET_APPS 1001
    while (true) {
        sdl_print_init(20, COLOR_WHITE, COLOR_BLACK);

        sdl_display_init(COLOR_BLACK);

        sdl_render_text(true, sdl_win_width/2, sdl_char_height/2, "Settings");

        loc = sdl_render_printf(false, 0, ROW2Y(2), "Devel_Mode = %ld", settings.devel_mode);
        sdl_register_event(loc, EVID_DEVEL_MODE);

        loc = sdl_render_printf(false, 0, ROW2Y(4), "Reset_Apps");
        sdl_register_event(loc, EVID_RESET_APPS);

        //int chh = sdl_char_height;
        DISPLAY_CONTROL_ITEM(2,"X",EVID_QUIT);

        sdl_display_present();

        event_id = sdl_get_event(-1);
        if (event_id == -1) {
            continue;
        }

        // process the event
        INFO("proc event_id %d\n", event_id);
        switch (event_id) {
        case EVID_DEVEL_MODE:
            settings.devel_mode = (settings.devel_mode ? 0 : 1);
            write_settings();
            break;
        case EVID_RESET_APPS:
            create_default_apps();
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

#define PORTNUM 9000   // IANA registered port range 1024 - 49151

static void *process_req_thread(void *cx);
static void process_req_using_android_sh(int sockfd, char *cmd);

static void *server_thread(void *cx)
{
    struct sockaddr_in server_address;
    int                listen_sockfd, ret;
    pthread_t          tid;

    sleep(1);
    INFO("SERVER_THREAD STARTING\n");

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
    server_address.sin_port        = htons(PORTNUM);
    ret = bind(listen_sockfd,
               (struct sockaddr *)&server_address,
               sizeof(server_address));
    if (ret == -1) {
        // xxx maybe retry, and server_thread_running not being set
        ERROR("bind, %s\n", strerror(errno));
        return NULL;
    }

    // listen 
    ret = listen(listen_sockfd, 5);
    if (ret == -1) {
        ERROR("listen, %s\n", strerror(errno));
        return NULL;
    }

    // accept and process connections
    INFO("accepting connections\n");
    server_thread_running = true;  // xxx needed?
    while (1) {
        int                sockfd;
        struct sockaddr_in peer_addr;
        socklen_t          peer_addr_len;
        char               peer_addr_str[200];

        // accept connection
        peer_addr_len = sizeof(peer_addr);
        sockfd = accept(listen_sockfd, (struct sockaddr *) &peer_addr, &peer_addr_len);
        if (sockfd == -1) {
            ERROR("accept, %s\n", strerror(errno));
            sleep(1);
            continue;
        }
        sock_addr_to_str(peer_addr_str, sizeof(peer_addr_str), (struct sockaddr *)&peer_addr);
        //INFO("accepted connection from %s, sockfd=%d\n", peer_addr_str, sockfd);

        // create thread to process the client request
        pthread_create(&tid, NULL, process_req_thread, (void*)(long)sockfd);
    }

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
            exit(1);
        }
        if (ch == '\n') {
            break;
        }
        *p++ = ch;
    }
    *p = '\0';
    //INFO("cmd '%s'\n", cmd);

    // some cmds are handled here, without using android /bin/sh
    if (strcmp(cmd, "log_mark") == 0) {
        INFO("---------- log_mark ----------\n");
        goto done;
    }
    if (strcmp(cmd, "log_clear") == 0) {
        freopen(log_file_pathname, "w", stdout);
        freopen(log_file_pathname, "w", stderr);
        setlinebuf(stdout);
        setlinebuf(stderr);
        INFO("---------- log_clear ----------\n");
        goto done;
    }
        
    // xxx comment
    if (fork() == 0) {
        process_req_using_android_sh(sockfd, cmd);
    }

done:
    close(sockfd);
    return NULL;
}

static void process_req_using_android_sh(int sockfd, char *cmd)
{
    char *argv[10];
    char cmd2[1000];

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

