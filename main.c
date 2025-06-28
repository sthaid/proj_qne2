#include <common.h>

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

const char *int_storage_path;

static char * sock_addr_to_str(char * s, int slen, struct sockaddr * addr);
static void process(int sockfd);

static void list_files(void)
{
    DIR *d = opendir(int_storage_path);
    if (d == NULL) {
        ERROR("opendir failed, %s\n", strerror(errno));
        return;
    }

    struct dirent * dirent;
    while ((dirent = readdir(d)) != NULL) {
        INFO("XXX %s\n", dirent->d_name);
    }

    closedir(d);
}

void *server_thread(void *cx)
{
    struct sockaddr_in server_address;
    int                listen_sockfd;
    int                ret;
    unsigned short     port = 1234;

    INFO("SERVER_THREAD STARTING\n");

    int_storage_path = SDL_AndroidGetInternalStoragePath();
    INFO("int_storage_path = %s\n", int_storage_path);
    list_files();

    // create socket
    listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sockfd == -1) {
        ERROR("socket, %s\n", strerror(errno));
    }

    // set socket options
    int reuseaddr = 1;
    ret = setsockopt(listen_sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseaddr, sizeof(reuseaddr));
    if (ret == -1) {
        ERROR("setsockopt SO_REUSEADDR, %s", strerror(errno));
    }

    // bind socket
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family      = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port        = htons(port);
    ret = bind(listen_sockfd,
               (struct sockaddr *)&server_address,
               sizeof(server_address));
    if (ret == -1) {
        ERROR("bind");
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
        process(sockfd);

        // xxx
        close(sockfd);
        INFO("done\n");
    }

    INFO("SERVER_THREAD TERMINATING\n");
    return NULL;
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

static void process(int sockfd)
{
    char s[1000], password[100], cmd[100], arg1[100], arg2[100], arg3[100];
    FILE *fpin=NULL, *fp=NULL;
    int line=0;

    fpin = fdopen(sockfd, "r+");
    // xxx check fpin, and other checks
    while (fgets(s, sizeof(s), fpin) != NULL) {
        line++;
        remove_trailing_crlf(s);
        INFO("XXX %d: '%s'\n", line, s);

        if (line == 1) {
            password[0] = cmd[0] = arg1[0] = arg2[0] = arg3[0] = '\0';
            if (sscanf(s, "password=%s cmd=%s arg1=%s arg2=%s arg3=%s", password, cmd, arg1, arg2, arg3) < 2) {
                ERROR("cmd line expected, got '%s'\n", s);
                break;
            }
            if (strcmp(password, "none") != 0) {
                ERROR("password invalid: got '%s', expected '%s'\n", password, "none");
                break;
            }
            INFO("password=%s cmd=%s arg1=%s arg2=%s arg3=%s\n", password, cmd, arg1, arg2, arg3);
            continue;
        }

        if (strcmp(cmd, "file_put") == 0) {
            char filename[200];
            sprintf(filename, "%s/%s", int_storage_path, arg1);

            // xxx check arg
            if (line == 2) {
                fp = fopen(filename, "w");
            }
            fprintf(fp, "%s\n", s);
        }
    }

    INFO("OKAY OKAY\n");
    fprintf(fpin, "OKAY\n");
    fflush(fpin);

    if (fpin) {
        fclose(fpin);
        fpin = NULL;
    }
    if (fp) {
        fclose(fp);
        fp = NULL;
    }
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

