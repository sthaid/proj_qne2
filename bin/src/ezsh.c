#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <libgen.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void set_fd_non_blocking(int fd);
void clear_fd_non_blocking(int fd);
int read_ez_cfg(char *ipaddr, int *port);
int write_loop(int fd, char *buff, int len);

int main(int argc, char **argv)
{
    struct sockaddr_in addr;
    socklen_t          addrlen;
    int                sockfd, ret, ret1, ret2;
    char               buff[10000];
    char              *cmd;
    int                port;
    char               ipaddr[100];

    // get ipaddr and portnum from esx.cfg file
    ret = read_ez_cfg(ipaddr, &port);
    if (ret == -1) {
        return 1;
    }

    // if arg is not provided for cmd then
    //   set cmd to '/bin/sh -i'
    // else
    //   set cmd to the arg provided
    // endif
    if (optind == argc) {
        cmd = "echo \"==== SHELL ====\"; /bin/sh -i";
    } else if (optind == argc-1) {
        cmd = argv[optind];
    } else {
        fprintf(stderr, "ERROR: invalid number of args %d\n", argc-optind);
        return 1;
    }

    // connect to android
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        fprintf(stderr, "ERROR: socket, %s\n", strerror(errno));
        return 1;
    }

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = inet_addr(ipaddr);
    addrlen = sizeof(addr);
    ret = connect(sockfd,  (struct sockaddr*)&addr, addrlen);
    if (ret != 0) {
        fprintf(stderr, "ERROR: connect %s:%d, %s\n", ipaddr, port, strerror(errno));
        return 1;
    }

    // set fd non blocking
    set_fd_non_blocking(STDIN_FILENO);
    set_fd_non_blocking(sockfd);

    // write cmd to android
    write(sockfd, cmd, strlen(cmd));
    write(sockfd, "\n", 1);

    // xxx cleanup and comment shut_wr
    bool shut_wr = false;

    // transfer data between the socket and stdin/stdout
    while (true) {
        // read from stdin and write to sockfd
        ret1 = read(STDIN_FILENO, buff, sizeof(buff));
        if (ret1 > 0) {
            write_loop(sockfd, buff, ret1);
        }
        if (ret1 == 0) {
            if (shut_wr == false) {
                shutdown(sockfd, SHUT_WR);
            }
            shut_wr = true;
        }

        // read from sockfd and write to stdout
        ret2 = read(sockfd, buff, sizeof(buff));
        if (ret2 == 0) {
            break;
        }
        if (ret2 > 0) {
            write_loop(STDOUT_FILENO, buff, ret2);
        }

        // sleep if connection is idle
        if (ret1 < 0 && ret2 < 0) {
            usleep(10000);
        }
    }

    // clear non blocking flag on stding
    clear_fd_non_blocking(STDIN_FILENO);

    // success
    return 0;
}

void set_fd_non_blocking(int fd)
{
    int flags;

    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        fprintf(stderr, "ERROR: failed to read flags of fd %d, %s\n", fd, strerror(errno));
        exit(1);
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        fprintf(stderr, "ERROR: failed to set flags of fd %d, flags=0x%x %s\n",
               fd, flags, strerror(errno));
        exit(1);
    }
}

void clear_fd_non_blocking(int fd)
{
    int flags;

    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        fprintf(stderr, "ERROR: failed to read flags of fd %d, %s\n", fd, strerror(errno));
        exit(1);
    }

    if (fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) == -1) {
        fprintf(stderr, "ERROR: failed to clear flags of fd %d, flags=0x%x %s\n",
               fd, flags, strerror(errno));
        exit(1);
    }
}

int read_ez_cfg(char *ipaddr, int *port)
{
    char self_path[100], ez_cfg_path[100], *self_dir, *p, s[100];;
    FILE *fp;

    // preset return config values
    ipaddr[0] = '\0';
    *port = 0;

    // get path to ez.cfg file
    readlink("/proc/self/exe", self_path, sizeof(self_path));
    self_dir = dirname(self_path);
    sprintf(ez_cfg_path, "%s/ez.cfg", self_dir);

    // open ez.cfg file
    fp = fopen(ez_cfg_path, "r");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: failed to open %s, %s\n", ez_cfg_path, strerror(errno));
        return -1;
    }

    // scan file for ipaddr:port
    while (fgets(s, sizeof(s), fp) != NULL) {
        // skip blank lines or lines begining with '#'
        if (s[0] == '\n' || s[0] == '#') {
            continue;
        }

        // extract ipaddr and port from string
        if ((p = strchr(s, ':')) == NULL) {
            // error, colon not found
            break;
        }
        *p = 0;
        strcpy(ipaddr, s);
        sscanf(p+1, "%d", port);
        break;
    }

    // close ez.cfg
    fclose(fp);

    // if ipaddr and port are not both set then return error
    if (ipaddr[0] == '\0' || *port == 0) {
        fprintf(stderr, "ERROR: failed to read ipaddr and port from %s\n", ez_cfg_path);
        fprintf(stderr, "ERROR: expected format 'xxx.xxx.xxx.xxx:nnnn'\n");
        return -1;
    }

    // success
    return 0;
}

int write_loop(int fd, char *buff, int len)
{
    int len_xfered = 0;
    int sleep_usecs = 0;
    int ret;

    while (len_xfered < len) {
        ret = write(fd, buff+len_xfered, len-len_xfered);
        if (ret > 0) {
            len_xfered += ret;
        } else {
            usleep(100000);  // 100 ms
            sleep_usecs += 100000;  // 100 ms
        }

        if (sleep_usecs > 10000000) {  // 10 seconds
            fprintf(stderr, "ERROR: write_loop timedout\n");
        }
    }

    return len_xfered;
}
