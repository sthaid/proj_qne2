#include <common.h>

// xxx update common.h

#include <SDL.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <dirent.h>

void *server_thread(void *cx);

int SDL_main(int argc, char **argv)
{
    int  w, h;
    uint32_t color;
    double inten;

    INFO("STARTING\n");

#if 0
    sdl_init(&w, &h);

    for (inten = .01; inten <= 1; inten += .01) {
        color = sdl_scale_color(COLOR_BLUE, inten);
        sdl_display_init(color);

        sdl_render_printf(0, 0, "%d %d", w, h);
        sdl_render_printf(0, h/2, "Q-%f", inten);

        sdl_display_present();
        usleep(100000);  // 100 ms
    }

    sdl_exit();
#endif

    pthread_t tid;
    pthread_create(&tid, NULL, server_thread, NULL);
    while (1) sleep(5);

    INFO("TERMINATING\n");
    return 0;
}

// ----------------- SERVER ----------------------------

// variables
const char *internal_storage_path;

// prototypes
static void process_client_req(int sockfd);
static void list_internal_storage_files(void);
static char * sock_addr_to_str(char * s, int slen, struct sockaddr * addr);
static void remove_trailing_crlf(char *s);

void *server_thread(void *cx)
{
    struct sockaddr_in server_address;
    int                listen_sockfd;
    int                ret;
    unsigned short     port = 1234; //xxx use define, and/or make adjustable

    INFO("SERVER_THREAD STARTING\n");

    // xxx
    internal_storage_path = SDL_AndroidGetInternalStoragePath();
    INFO("internal_storage_path = %s\n", internal_storage_path);

    // xxx
    list_internal_storage_files();

    // create listen socket
    listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sockfd == -1) {
        ERROR("socket, %s\n", strerror(errno));
    }

    // set socket options  xxx what is this for
    int reuseaddr = 1;
    ret = setsockopt(listen_sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseaddr, sizeof(reuseaddr));
    if (ret == -1) {
        ERROR("setsockopt SO_REUSEADDR, %s", strerror(errno));
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
        ERROR("bind");  // xxx newlines
    }

    // listen 
    ret = listen(listen_sockfd, 5);
    if (ret == -1) {
        ERROR("listen");
    }

    // accept and process connections
    INFO("accepting connections\n");
    while (1) {
        int                  sockfd;
        socklen_t            peer_address_len;
        struct sockaddr_in   peer_address;
        char                 peer_addr_str[200];

        // accept connection
        peer_address_len = sizeof(peer_address);
        sockfd = accept(listen_sockfd, (struct sockaddr *) &peer_address, &peer_address_len);
        if (sockfd == -1) {
            ERROR("accept, %s\n", strerror(errno));
            sleep(1);
            continue;
        }

        // print peer addr
        sock_addr_to_str(peer_addr_str, sizeof(peer_addr_str), (struct sockaddr *)&peer_address);
        INFO("accepted connection from %s\n", peer_addr_str);

        // xxx
        process_client_req(sockfd);

        // xxx
        close(sockfd);
        INFO("done\n");
    }

    INFO("SERVER_THREAD TERMINATING\n");
    return NULL;
}

// xxx check sockfp, and other checks
static void process_client_req(int sockfd)
{
    FILE *sockfp=NULL;
    char  password[100], cmd[100], arg1[100], arg2[100];
    char  err_str[100]="";

    // xxx
    sockfp = fdopen(sockfd, "r+");
    if (sockfp == NULL) {
        ERROR("failed to fdopen sockfp, %s\n", strerror(errno));
        return;
    }

    // read first line from client
    password[0] = cmd[0] = arg1[0] = arg2[0] = '\0';
    if (fscanf(sockfp, "password=%s cmd=%s arg1=%s arg2=%s", password, cmd, arg1, arg2) < 2) {
        sprintf(err_str, "bad request");
        goto error;
    }

    // validate password xxx tbd
    if (strcmp(password, "none") != 0) {
        sprintf(err_str, "invalid password");
        goto error;
    }

    // process cmd
    INFO("processing cmd=%s arg1=%s arg2=%s\n", cmd, arg1, arg2);
    if (strcmp(cmd, "file_put") == 0) {
        // copy file to internal storage
        FILE *fp;
        char  filename[100], s[1000];

        if (arg1[0] == '\0') {
            sprintf(err_str, "file_put, filename not provied\n");
            goto error;
        }

        sprintf(filename, "%s/%s", internal_storage_path, arg1);
        fp = fopen(filename, "w");
        if (fp == NULL) {
            sprintf(err_str, "failed to open %s for writing, %s\n", filename, strerror(errno));
            goto error;
        }
        while (fgets(s, sizeof(s), sockfp) != NULL) {
            fputs(s, fp);
        }
        fclose(fp);
    } else if (strcmp(cmd, "file_get") == 0) {
        // print contents of internal storage file
        FILE *fp;
        char  filename[100], s[1000];

        if (arg1[0] == '\0') {
            sprintf(err_str, "file_get, filename not provied\n");
            goto error;
        }

        sprintf(filename, "%s/%s", internal_storage_path, arg1);
        fp = fopen(filename, "r");
        if (fp == NULL) {
            sprintf(err_str, "failed to open %s for reading, %s\n", filename, strerror(errno));
            goto error;
        }
        while (fgets(s, sizeof(s), fp) != NULL) {
            fputs(s, sockfp);
        }
        fclose(fp);
    } else if (strcmp(cmd, "file_rm") == 0) {
        // remove internel storage file
    } else if (strcmp(cmd, "file_ls") == 0) {
        // list files in internal storage
    } else {
        // invalid command
    }

    fprintf(sockfp, "OKAY\n");
    fclose(sockfp);
    return;

error:
    fprintf(sockfp, "%s\n", err_str);
    fclose(sockfp);
    return;
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
