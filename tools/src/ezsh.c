#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORTNUM 9000
#define IP_ADDR "192.168.1.243"  //xxx
//#define IP_ADDR "127.0.0.1"

void set_fd_non_blocking(int fd);
void clear_fd_non_blocking(int fd);

int main(int argc, char **argv)
{
    struct sockaddr_in addr;
    socklen_t          addrlen;
    int                sockfd, ret, ret1, ret2;
    char               buff[10000];
    char             * cmd;

    // if arg is not provided for cmd then
    //   set cmd to '/bin/sh -i'
    // else
    //   set cmd to the arg provided
    // endif
    if (optind == argc) {
        cmd = "echo \"==== ANDROID SHELL ====\"; /bin/sh -i";
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
    addr.sin_port        = htons(PORTNUM);
    addr.sin_addr.s_addr = inet_addr(IP_ADDR);
    addrlen = sizeof(addr);
    ret = connect(sockfd,  (struct sockaddr*)&addr, addrlen);
    if (ret != 0) {
        fprintf(stderr, "ERROR: connect, %s\n", strerror(errno));
        return 1;
    }

    // set fd non blocking
    set_fd_non_blocking(STDIN_FILENO);
    set_fd_non_blocking(sockfd);

    // write cmd to android
    write(sockfd, cmd, strlen(cmd));
    write(sockfd, "\n", 1);

    bool shut_wr = false;

    // transfer data between the socket and stdin/stdout
    while (true) {
        // read from stdin and write to sockfd
        ret1 = read(STDIN_FILENO, buff, sizeof(buff));
        if (ret1 > 0) {
            write(sockfd, buff, ret1);
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
            write(STDOUT_FILENO, buff, ret2);
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
