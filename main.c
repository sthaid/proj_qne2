#include <std_hdrs.h>

#include <sdl.h>
#include <utils.h>

// variables
const char *internal_storage_path;
bool        server_thread_running;
char        log_file_pathname[100];

// prototypes 
static void controller(void);
static void *server_thread(void *cx);

// routines to launch a C program using picoc interpreter
void picoc_bg(char *args);
int picoc_fg(char *args);

// -----------------  MAIN  ------------------------------------------

int SDL_main(int argc, char **argv)
{
    FILE *fp;
    pthread_t tid;

    // init logging
    internal_storage_path = SDL_AndroidGetInternalStoragePath();
    sprintf(log_file_pathname, "%s/%s", internal_storage_path, "log");

    freopen(log_file_pathname, "a", stdout);
    freopen(log_file_pathname, "a", stderr);
    setlinebuf(stdout);
    setlinebuf(stderr);

    // print startup messages
    INFO("=====================================================\n");  // xxx include version
    INFO("internal_storage_path = %s\n", internal_storage_path);

    // xxx
    chdir(internal_storage_path);

    // create server thread
    pthread_create(&tid, NULL, server_thread, NULL);
    while (server_thread_running == false) {
        usleep(10000);
    }

    // xxx comment
    controller();

    // end program
    // xxx kill the server thread ?
    INFO("TERMINATING\n");
    return 0;
}

// -----------------  CONTROLLER  ------------------------------------

#define MAX_MENU 18

typedef struct {
    char name[16];
    char *args;
} menu_t;

static menu_t menu[MAX_MENU];

static void display_menu(int w, int h);
static void read_menu(void);

static void controller(void)
{
    int w, h, event_id, rc;

    rc = sdl_init(&w, &h); //xxx handle ret

    while (true) {
        // xxx reset other stuff here too, fontsz, color
        sdl_display_init(COLOR_PURPLE);

        // display menu, and register for sdl events
        display_menu(w, h);

        // update the display
        sdl_display_present();

        // wait for an event
        event_id = sdl_get_event(1000000);
        if (event_id == -1) {
            continue;
        }

        // process the event
        INFO("proc event_id %d\n", event_id);
        if (event_id < 0 || event_id >= MAX_MENU) {
            ERROR("unexpected event_id %d\n", event_id);
        } else if (strcmp(menu[event_id].name, "end") == 0) {
            INFO("GOT EXIT CMD\n");
            break;
        } else {
            INFO("running %s\n", menu[event_id].name);
            rc = picoc_fg(menu[event_id].args);
            INFO("done %s, rc=%d\n", menu[event_id].name, rc);
        }
    }

    sdl_exit();
}

static void display_menu(int w, int h)
{
    int id, x, xx, y, yy;
    sdl_rect_t *loc;

    // xxx
    read_menu();

    // xxx comment
    for (id = 0; id < MAX_MENU; id++) {
        if (menu[id].name[0] == '\0') {
            continue;
        }

        // xxx add shape, maybe circle, or rounded off square

        // xxx cleanup
        x = id % 3;
        xx = (w/3)/2 + x * (w/3);

        y = id / 3;
        yy = (h/6)/2 + y * (h/6);

        loc = sdl_render_text(xx, yy, menu[id].name);

        sdl_register_event(loc, id);
    }
}

static void read_menu(void)
{
    FILE *fp;
    char s[1000], name[32], *args;
    int cnt, id, n, ret;
    struct stat statbuf;

    static long menu_mtime;
    static char menu_path[100];

    // construct menu_path, if not already done so
    if (menu_path[0] == '\0') {
        sprintf(menu_path, "%s/menu", internal_storage_path);
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
    for (id = 0; id < MAX_MENU; id++) {
        free(menu[id].args);
    }
    memset(menu, 0, sizeof(menu));

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

        // extract id, name, and args:
        // example:
        //   s = "5 test : test_main.c test2.c"
        // scanf result:
        //   id   = 5
        //   name = test
        //   args = test_main.c test2.c
        name[0] = '\0';
        id = n = 0;
        cnt = sscanf(s, "%d %s : %n", &id, name, &n);
        if (cnt < 2) {
            ERROR("invalid line in menu file, '%s'\n", s);
            continue;
        }
        args = (n > 0 ? s+n : NULL);

        // save name and args in menu struct
        strcpy(menu[id].name, name);
        if (args != NULL) {
            menu[id].args = strdup(args);
        }
    }

    // close menu file
    fclose(fp);

    // debug print the new menu
    INFO("menu is now ...\n");
    for (id = 0; id < MAX_MENU; id++) {
        if (menu[id].name[0] != '\0') {
            INFO("%4d %8s : %s\n", id, menu[id].name, menu[id].args);
        }
    }
}

// ----------------- SERVER ----------------------------

#define PORTNUM 1234

static void *process_req_thread(void *cx);
static void process_req_using_android_sh(int sockfd, char *cmd);

static void *server_thread(void *cx)
{
    struct sockaddr_in server_address;
    int                listen_sockfd, ret;
    pthread_t          tid;

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
    server_thread_running = true;
    while (1) {
        int                sockfd;
        struct sockaddr_in peer_addr;
        socklen_t          peer_addr_len;
        char               peer_addr_str[200];
        int                ret;

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

    sprintf(cmd2, "cd %s; %s", internal_storage_path, cmd);
    argv[0] = "/bin/sh";
    argv[1] = "-c";
    argv[2] = cmd2;
    argv[3] = NULL;

    execv("/bin/sh", argv);

    // not reached
    exit(1);
}

