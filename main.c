// xxx update common.h
// - or dont use common.h

#include <common.h>

#include <SDL.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <unistd.h>

// variables
const char *internal_storage_path;

char        log_filename[100];
int         log_cat_size;
bool        log_cat_is_running;

bool        server_thread_running;

// prototyes xxx organize
static void list_internal_storage_files(void);
void run_prog(bool bg);
void *server_thread(void *cx);
static char * sock_addr_to_str(char * s, int slen, struct sockaddr * addr);
static void remove_trailing_crlf(char *s);
static int logfile_size(void);
static bool is_socket_connected(int socket_fd);
static void get_file_info(char *name, int *size, char *mtime);

// -----------------  MAIN  ------------------------------------------

int SDL_main(int argc, char **argv)
{
    int  w, h;
    FILE *fp;
    pthread_t tid;

    // init logging
    internal_storage_path = SDL_AndroidGetInternalStoragePath();
    sprintf(log_filename, "%s/%s", internal_storage_path, "log");
    log_cat_size = logfile_size();

    freopen(log_filename, "a", stdout);
    freopen(log_filename, "a", stderr);
    setlinebuf(stdout);
    setlinebuf(stderr);

    // print startup messages
    INFO("=====================================================\n");  // xxx include version
    INFO("internal_storage_path = %s\n", internal_storage_path);
    list_internal_storage_files();

    // create server thread
    pthread_create(&tid, NULL, server_thread, NULL);
    while (server_thread_running == false) {
        usleep(10000);
    }

#if 0
    // xxx sdl test
    uint32_t color;
    double inten;

    // xxx sdl
    sdl_init(&w, &h);

    for (inten = .01; inten <= 1; inten += .01) {
        color = sdl_scale_color(COLOR_BLUE, inten);
        sdl_display_init(color);

        sdl_render_printf(0, 0, "%d %d", w, h);
        sdl_render_printf(0, h/2, "Q-%f", inten);

        sdl_display_present();
        usleep(100000);  // 100 ms
    }

    // sdl_exit
#endif

    // run 2 instances of test picoc program
    run_prog(true);    // bg
    run_prog(false);   // fg

    // end program
    INFO("TERMINATING\n");
    return 0;
}

// ----------------- SERVER ----------------------------

typedef struct {
    FILE *sockfp;
    int  sockfd;
    char password[100];
    char cmd[100];
    char arg1[100];
    char arg2[100];
    char peer_addr_str[100];
} req_t;

#define DEFAULT_PORT 1234

// prototypes
static void *process_client_req(void *cx);

void *server_thread(void *cx)
{
    struct sockaddr_in server_address;
    int                listen_sockfd, ret;
    unsigned short     port = DEFAULT_PORT; //xxx make adjustable?

    INFO("SERVER_THREAD STARTING\n");

    // create listen socket
    listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sockfd == -1) {
        ERROR("socket, %s\n", strerror(errno));
        return NULL;
    }

    // set socket options, xxx explain
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

        // xxx
        sockfp = fdopen(sockfd, "r+");
        if (sockfp == NULL) {
            ERROR("failed to fdopen sockfp, %s\n", strerror(errno));
            close(sockfd);
            sleep(1);
            continue;
        }

        // read first line from client, which must contain, at least, password, and cmd
        int    cnt;
        req_t *req;

        req = calloc(1, sizeof(req_t)); //xxx check req
        sock_addr_to_str(req->peer_addr_str, sizeof(req->peer_addr_str), (struct sockaddr *)&peer_address);
        cnt = fscanf(sockfp, "qne %s %s %s %s", req->password, req->cmd, req->arg1, req->arg2);
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

        // process the client request
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
    // extract fields from req_t arg
    req_t *req    = (req_t*)cx;
    FILE  *sockfp = req->sockfp;
    int    sockfd = req->sockfd;
    char  *cmd    = req->cmd;
    char  *arg1   = req->arg1;
    char  *arg2   = req->arg2;
    char  *peer_addr_str = req->peer_addr_str;

    // declare more variables
    char  err_str[100]="";

    INFO("received cmd=%s arg1=%s arg2=%s, from %s\n", cmd, arg1, arg2, peer_addr_str);

    // process cmd
    if (strcmp(cmd, "put") == 0) {
        // copy file to internal storage
        FILE *fp;
        char  filename[100], s[1000];

        if (arg1[0] == '\0') {
            sprintf(err_str, "put: filename required");
            goto error;
        }

        sprintf(filename, "%s/%s", internal_storage_path, arg1);
        fp = fopen(filename, "w");
        if (fp == NULL) {
            sprintf(err_str, "put: failed to open %s for writing, %s", filename, strerror(errno));
            goto error;
        }
        while (fgets(s, sizeof(s), sockfp) != NULL) {
            fputs(s, fp);
        }
        fclose(fp);
    } else if (strcmp(cmd, "get") == 0) {
        // print contents of internal storage file
        FILE *fp;
        char  filename[100], s[1000];

        if (arg1[0] == '\0') {
            sprintf(err_str, "get: filename required");
            goto error;
        }

        sprintf(filename, "%s/%s", internal_storage_path, arg1);
        fp = fopen(filename, "r");
        if (fp == NULL) {
            sprintf(err_str, "get: failed to open %s for reading, %s", filename, strerror(errno));
            goto error;
        }
        while (fgets(s, sizeof(s), fp) != NULL) {
            fputs(s, sockfp);
        }
        fclose(fp);
    } else if (strcmp(cmd, "rm") == 0) {
        // remove internel storage file
        char filename[100];
        int  ret;

        if (arg1[0] == '\0') {
            sprintf(err_str, "rm: filename required");
            goto error;
        }

        sprintf(filename, "%s/%s", internal_storage_path, arg1);
        ret = unlink(filename);
        if (ret != 0) {
            sprintf(err_str, "file_del, unlink failed, %s", strerror(errno));
            goto error;
        }
    } else if (strcmp(cmd, "ls") == 0) {
// xxx add info
        // list files in internal storage
        DIR *dir = opendir(internal_storage_path);
        struct dirent * dirent;

        if (dir == NULL) {
            sprintf(err_str, "ls: opendir failed for %s, %s", 
                    internal_storage_path, strerror(errno));
            goto error;
        }

        while ((dirent = readdir(dir)) != NULL) {
            if (strcmp(dirent->d_name, ".") == 0 ||
                strcmp(dirent->d_name, "..") == 0)
            {
                continue;
            }

            char *name = dirent->d_name;
            int size;
            char mtime[100];
            char filename[200];
            sprintf(filename, "%s/%s", internal_storage_path, name);
            get_file_info(filename, &size, mtime);

            fprintf(sockfp, "%8d %s %s\n", size, mtime, name);
        }

        closedir(dir);
    } else if (strcmp(cmd, "logcat") == 0) {
        int fd;
        int size, xfer_len, ret;
        char buff[10000];

// xxx error  checks
        if (log_cat_is_running) {
            sprintf(err_str, "logcat: is already running");
            goto error;
        }

        log_cat_is_running = true;

        fd = open(log_filename, O_RDONLY);
        while (true) {
            // get size of logfile
            size = logfile_size();
            if (size < 0) {
                break;
            }

            // if size is greater than before then read the new data and 
            // write the new data to sockfd
            while (size > log_cat_size) {
                xfer_len = size - log_cat_size;
                if (xfer_len > sizeof(buff)) {
                    xfer_len = sizeof(buff);
                }

                lseek(fd, log_cat_size, SEEK_SET);
                read(fd, buff, xfer_len);
                fwrite(buff, 1, xfer_len, sockfp);
                ret = fflush(sockfp);
                if (ret != 0) {
                    INFO("XXXXXXXXXXXXXXXXX NOT CONNECTED FFLUSH\n");
                    break;
                }

                log_cat_size += xfer_len;
            }

            // short sleep, 10ms
            usleep(10000);

            // if socket not connected then break
            if (is_socket_connected(sockfd) == false) {
                INFO("XXXXXXXXXXXXXXXXX NOT CONNECTED\n");
                break;
            }
        }

        log_cat_is_running = false;
    } else {
        sprintf(err_str, "%s: is not a valid cmd", cmd);
        goto error;
    }

    INFO("success cmd=%s\n", cmd);
    fprintf(sockfp, "%s: success\n", cmd);
    fclose(sockfp);
    close(sockfd);
    free(req);
    return NULL;

error:
    INFO("failed cmd=%s\n", cmd);
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

        INFO("%s\n", dirent->d_name);  // xxx other fields too
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

static int logfile_size(void)
{
    int ret;
    struct stat statbuf;

    ret = lstat(log_filename, &statbuf);
    if (ret < 0) {
        return -1;
    }
    return statbuf.st_size;
}

static bool is_socket_connected(int socket_fd)
{
    int error = 0;
    int ret;
    socklen_t len = sizeof(error);

    ret = getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &error, &len);

    return ret == 0 && error == 0;
}

static void get_file_info(char *name, int *size, char *mtime)
{
    int ret;
    struct stat statbuf;

    *size = 0;
    mtime[0] = '\0';

    ret = lstat(name, &statbuf);
    if (ret < 0) {
        return;
    }

    *size = statbuf.st_size;
    ctime_r(&statbuf.st_mtime, mtime);

    remove_trailing_crlf(mtime);
}
