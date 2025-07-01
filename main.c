#include <std_hdrs.h>
#include <sdl.h>
#include <utils.h>

// variables
const char *internal_storage_path;
char        log_file_pathname[100];
int         log_cat_size;
bool        log_cat_is_running;
bool        server_thread_running;

// prototypes
static void sdl_test(void);
void run_prog(bool bg);
void *server_thread(void *cx);

// support routine prototypes
static void list_internal_storage_files(void);
static char * sock_addr_to_str(char * s, int slen, struct sockaddr * addr);
static void remove_trailing_crlf(char *s);
static bool is_socket_connected(int socket_fd);
static int get_file_size(char *pathname);
static void get_file_info(char *pathname, int *size, char *mtime);

// -----------------  MAIN  ------------------------------------------

int SDL_main(int argc, char **argv)
{
    FILE *fp;
    pthread_t tid;

    // init logging
    internal_storage_path = SDL_AndroidGetInternalStoragePath();
    sprintf(log_file_pathname, "%s/%s", internal_storage_path, "log");
    log_cat_size = get_file_size(log_file_pathname);

    freopen(log_file_pathname, "a", stdout);
    freopen(log_file_pathname, "a", stderr);
    setlinebuf(stdout);
    setlinebuf(stderr);

    // print startup messages
    INFO("=====================================================\n");  // xxx include version
    INFO("internal_storage_path = %s\n", internal_storage_path);
    list_internal_storage_files();

    printf("sizeof(long) = %zd\n", sizeof(long));
    printf("sizeof(long long) = %zd\n", sizeof(long long));

    // create server thread
    pthread_create(&tid, NULL, server_thread, NULL);
    while (server_thread_running == false) {
        usleep(10000);
    }

    // xxx
    sdl_test();

#if 0
    // run 2 instances of test picoc program
    run_prog(true);    // bg
    run_prog(false);   // fg
#endif

    // xxx
    while (true) pause();

    // end program
    INFO("TERMINATING\n");
    return 0;
}

// -----------------  SDL TEST ---------------------------------------

static void sdl_test(void)
{
    // xxx sdl test
    uint32_t color;
    double inten;
    int char_width, char_height;
    sdl_rect_t loc;
    int  w, h;
    bool event_processed;

    // xxx sdl
    sdl_init(&w, &h);
    INFO("window size %d %d\n", w, h);

    sdl_get_char_size(&char_width, &char_height);  // xxx also return ptsize
    INFO("char_width/height = %d %d\n", char_width, char_height);

    color = sdl_scale_color(COLOR_BLUE, 0.25);

    while (true) {
        sdl_display_init(color);

        loc = sdl_render_printf(0,200, "%s", "HELLO");
        //INFO("loc = %d %d %d %d\n", loc.x, loc.y, loc.w, loc.h);
        sdl_register_event(loc, 1);

        sdl_display_present();

        // xxx simplify
        event_processed = false;
        while (true) {
            int event_id = sdl_get_event();
            if (event_id == 0) {
                break;
            }

            INFO("processing event %d\n", event_id);
            event_processed = true;
            if (event_id == 1) {
                run_prog(false);
            }
        }

        if (event_processed) {
            usleep(1000);
        } else {
            usleep(1000000);
        }
    }

    sdl_exit();
}

// ----------------- SERVER ----------------------------

// defines
#define DEFAULT_SERVER_PORT 1234

// typedefs
typedef struct {
    FILE *sockfp;
    int  sockfd;
    char password[100];
    char cmd[100];
    char filename[100];
    char peer_addr_str[100];
} server_req_t;

// prototypes
static void *process_client_req(void *cx);

void *server_thread(void *cx)
{
    struct sockaddr_in server_address;
    int                listen_sockfd, ret;
    unsigned short     port = DEFAULT_SERVER_PORT; //xxx make adjustable?
    int                cnt;
    server_req_t      *req;
    char               s[200];

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
    server_address.sin_port        = htons(port);
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
        FILE              *sockfp;
        socklen_t          peer_address_len;
        struct sockaddr_in peer_address;

        // accept connection
        peer_address_len = sizeof(peer_address);
        sockfd = accept(listen_sockfd, (struct sockaddr *) &peer_address, &peer_address_len);
        if (sockfd == -1) {
            ERROR("accept, %s\n", strerror(errno));
            sleep(1);
            continue;
        }

        // open fp for sockfd
        sockfp = fdopen(sockfd, "r+");
        if (sockfp == NULL) {
            ERROR("failed to fdopen sockfp, %s\n", strerror(errno));
            close(sockfd);
            sleep(1);
            continue;
        }

        // read first line from client, which must contain, at least, password, and cmd
        req = calloc(1, sizeof(server_req_t));
        sock_addr_to_str(req->peer_addr_str, sizeof(req->peer_addr_str), (struct sockaddr *)&peer_address);
        fgets(s, sizeof(s), sockfp);
        cnt = sscanf(s, "qne %s %s %s", req->password, req->cmd, req->filename);
        if (cnt < 2) {
            fprintf(sockfp, "bad request");
            fclose(sockfp);
            close(sockfd);
            free(req);
            sleep(1);
            continue;
        }

        // validate password xxx tbd
        if (strcmp(req->password, "none") != 0) {
            fprintf(sockfp, "invalid password");
            fclose(sockfp);
            close(sockfd);
            free(req);
            sleep(1);
            continue;
        }

        // process the request; run logcat request in a thread because the
        // logcat request does not complete
        req->sockfp = sockfp;
        req->sockfd = sockfd;
        if (strcmp(req->cmd, "logcat") == 0) {
            pthread_t tid;
            pthread_create(&tid, NULL, process_client_req, req);
        } else {
            process_client_req(req);
        }
    }

    INFO("SERVER_THREAD TERMINATING\n");
    return NULL;
}

static void *process_client_req(void *cx)
{
    // extract fields from server_req_t arg
    server_req_t *req    = (server_req_t*)cx;
    FILE  *sockfp        = req->sockfp;
    int    sockfd        = req->sockfd;
    char  *cmd           = req->cmd;
    char  *filename      = req->filename;
    char  *peer_addr_str = req->peer_addr_str;

    // declare more variables
    char  err_str[200] = "";
    char  pathname[200] = "";

    INFO("received %s %s, from %s\n", cmd, filename, peer_addr_str);

    // the put, get, and rm cmds require filename
    if (strcmp(cmd, "put") == 0 ||
        strcmp(cmd, "get") == 0 ||
        strcmp(cmd, "rm") == 0)
    {
        if (filename[0] == '\0') {
            sprintf(err_str, "%s: filename required", cmd);
            goto error;
        }
        sprintf(pathname, "%s/%s", internal_storage_path, filename);
    }

    // process cmd
    if (strcmp(cmd, "put") == 0) {
        // copy file to internal storage
        FILE *fp;
        char  s[1000];

        fp = fopen(pathname, "w");
        if (fp == NULL) {
            sprintf(err_str, "put: failed to open %s for writing, %s", pathname, strerror(errno));
            goto error;
        }
        while (fgets(s, sizeof(s), sockfp) != NULL) {
            fputs(s, fp);
        }
        fclose(fp);
    } else if (strcmp(cmd, "get") == 0) {
        // print contents of internal storage file
        FILE *fp;
        char  s[1000];

        fp = fopen(pathname, "r");
        if (fp == NULL) {
            sprintf(err_str, "get: failed to open %s for reading, %s", pathname, strerror(errno));
            goto error;
        }
        while (fgets(s, sizeof(s), fp) != NULL) {
            fputs(s, sockfp);
        }
        fclose(fp);
    } else if (strcmp(cmd, "rm") == 0) {
        // remove internel storage file
        if (unlink(pathname) != 0) {
            sprintf(err_str, "rm, unlink %s failed, %s", pathname, strerror(errno));
            goto error;
        }
    } else if (strcmp(cmd, "ls") == 0) {
        // list files in internal storage
        DIR *dir = opendir(internal_storage_path);
        struct dirent * dirent;

        if (dir == NULL) {
            sprintf(err_str, "ls: opendir failed for %s, %s", 
                    internal_storage_path, strerror(errno));
            goto error;
        }

        while ((dirent = readdir(dir)) != NULL) {
            // skip the "." and ".." directories
            if (strcmp(dirent->d_name, ".") == 0 ||
                strcmp(dirent->d_name, "..") == 0)
            {
                continue;
            }

            // get file info
            char *name = dirent->d_name;
            int size;
            char mtime[100];
            char pathname[200];
            sprintf(pathname, "%s/%s", internal_storage_path, name);
            get_file_info(pathname, &size, mtime);

            // print info to sockfp
            fprintf(sockfp, "%8d %s %s\n", size, mtime, name);
        }

        closedir(dir);
    } else if (strcmp(cmd, "logcat") == 0) {
        int  fd, size, xfer_len;
        char buff[10000];

        // if logcat is already running then return error
        if (log_cat_is_running) {
            sprintf(err_str, "logcat: is already running");
            goto error;
        }

        // keep track of logcat running
        log_cat_is_running = true;

        // open logfile
        fd = open(log_file_pathname, O_RDONLY);
        if (fd < 0) {
            sprintf(err_str, "logcat: failed to open %s, %s", log_file_pathname, strerror(errno));
            goto error;
        }

        // copy additions to logfile to the sockfp
        while (true) {
            // get size of logfile
            size = get_file_size(log_file_pathname);
            if (size < 0) {
                sprintf(err_str, "logcat: failed to get size of %s, %s", log_file_pathname, strerror(errno));
                close(fd);
                goto error;
            }

            // if size has increased then
            // copy the new logfile data to the sockfp
            while (size > log_cat_size) {
                // determine size to transfer from logfile to sockfp
                xfer_len = size - log_cat_size;
                if (xfer_len > sizeof(buff)) {
                    xfer_len = sizeof(buff);
                }

                // read from logfile, write to sockfp
                // xxx checks may be needed here
                lseek(fd, log_cat_size, SEEK_SET);
                read(fd, buff, xfer_len);
                fwrite(buff, 1, xfer_len, sockfp);
                fflush(sockfp);

                // update the size of the logfile that has been xfered to sockfp
                log_cat_size += xfer_len;
            }

            // short sleep, 10ms
            usleep(10000);

            // if socket not connected then break
            if (is_socket_connected(sockfd) == false) {
                break;
            }
        }

        // close logfile
        close(fd);

        // keep track of logcat running
        log_cat_is_running = false;
    } else {
        // invalid cmd supplied
        sprintf(err_str, "%s: is not a valid cmd", cmd);
        goto error;
    }

    // success
    INFO("success %s\n", cmd);
    fprintf(sockfp, "%s: success\n", cmd);
    fclose(sockfp);
    close(sockfd);
    free(req);
    return NULL;

error:
    // failure
    INFO("failed %s\n", cmd);
    fprintf(sockfp, "%s\n", err_str);
    fclose(sockfp);
    close(sockfd);
    free(req);
    return NULL;
}

// ----------------- SUPPORT ---------------------------

static void list_internal_storage_files(void)
{
    DIR *dir = opendir(internal_storage_path);
    struct dirent * dirent;

    if (dir == NULL) {
        ERROR("opendir failed, %s\n", strerror(errno));
        return;
    }

    while ((dirent = readdir(dir)) != NULL) {
        if (strcmp(dirent->d_name, ".") == 0 ||
            strcmp(dirent->d_name, "..") == 0)
        {
            continue;
        }

        // get file info
        char *name = dirent->d_name;
        int size;
        char mtime[100];
        char pathname[200];
        sprintf(pathname, "%s/%s", internal_storage_path, name);
        get_file_info(pathname, &size, mtime);

        // print info to sockfp
        INFO("%8d %s %s\n", size, mtime, name);
    }

    closedir(dir);
}

static char * sock_addr_to_str(char * s, int slen, struct sockaddr * addr)
{
    char addr_str[100];
    int port2;

    if (addr->sa_family == AF_INET) {
        inet_ntop(AF_INET,
                  &((struct sockaddr_in*)addr)->sin_addr,
                  addr_str, sizeof(addr_str));
        port2 = ((struct sockaddr_in*)addr)->sin_port;
    } else if (addr->sa_family == AF_INET6) {
        inet_ntop(AF_INET6,
                  &((struct sockaddr_in6*)addr)->sin6_addr,
                 addr_str, sizeof(addr_str));
        port2 = ((struct sockaddr_in6*)addr)->sin6_port;
    } else {
        snprintf(s,slen,"Invalid AddrFamily %d", addr->sa_family);
        return s;
    }

    snprintf(s,slen,"%s:%d",addr_str,ntohs(port2));
    return s;
}

static void remove_trailing_crlf(char *s)
{
    int len = strlen(s);

    if (len > 0 && (s[len-1] == '\n' || s[len-1] == '\r')) {
        s[len-1] = '\0';
        len--;
    } else {
        return;
    }

    if (len > 0 && (s[len-1] == '\n' || s[len-1] == '\r')) {
        s[len-1] = '\0';
        len--;
    }
}

static bool is_socket_connected(int socket_fd)
{
    int error = 0;
    int ret;
    socklen_t len = sizeof(error);

    ret = getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &error, &len);

    return ret == 0 && error == 0;
}

static int get_file_size(char *pathname)
{
    int ret;
    struct stat statbuf;

    ret = lstat(pathname, &statbuf);
    if (ret < 0) {
        return -1;
    }
    return statbuf.st_size;
}

static void get_file_info(char *pathname, int *size, char *mtime)
{
    int ret;
    struct stat statbuf;

    *size = 0;
    mtime[0] = '\0';

    ret = lstat(pathname, &statbuf);
    if (ret < 0) {
        return;
    }

    *size = statbuf.st_size;
    ctime_r(&statbuf.st_mtime, mtime);

    remove_trailing_crlf(mtime);
}
