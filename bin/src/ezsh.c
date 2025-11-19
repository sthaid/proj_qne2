#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>

#include <sys/types.h>  // xxx all needed?
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>

#include <readline/readline.h>
#include <readline/history.h>

// info from ez.cfg
char ipaddr[100];
int  port;
char password[100];


char *prompt = "ezsh> ";  // include cwd
char cwd[100];
int sockfd;  // private
FILE *sockfp;

// prototypes
void remove_leading_spaces(char *s);
void read_ez_cfg(void);
void connect_to_android(void);
void run_cmd_on_android(char *cmdline);

// xxx todo
// - cd and pwd cmds
//   - prepend the current cd value
// - q cmd
// - ls alias
// - END_OF_DATA marker
// - copy file to/from android
// - setting for password, require it be changed , use change_me
// - vi cmd
// - clean up code and comments
// -  use strings of either size 100 or 1000

// xxx done
// - hist

// -----------------  MAIN  -------------------------------------------------

int main(int argc, char **argv)
{
    char *cmdline;

    // read ez.cfg file, to get ipaddr, port, and password
    read_ez_cfg();

    // connect to android: also validates password and gets curr-working-dir
    connect_to_android();

    // runtime loop
    while (true) {
        // read cmdline
        cmdline = readline(prompt);
        if (cmdline == NULL) {
            break;
        }
        remove_leading_spaces(cmdline);
        if (cmdline[0] == '\0') {
            continue;
        }
        add_history(cmdline);

        // process the cmdline
        run_cmd_on_android(cmdline);

        // free cmdline
        free(cmdline);
    }
}

void read_ez_cfg(void)
{
    char  self_path[100], ez_cfg_path[100], *self_dir, s[100];
    FILE *fp;
    int   cnt;

    // get path to ez.cfg file
    readlink("/proc/self/exe", self_path, sizeof(self_path));
    self_dir = dirname(self_path);
    sprintf(ez_cfg_path, "%s/ez.cfg", self_dir);

    // open ez.cfg file
    fp = fopen(ez_cfg_path, "r");
    if (fp == NULL) {
        printf("ERROR: failed to open %s, %s\n", ez_cfg_path, strerror(errno));
        exit(1);
    }

    // read the ipaddr:port and password, which must be on the first line of ez.cfg
    fgets(s, sizeof(s), fp);
    cnt = sscanf(s, "%s %d %s", ipaddr, &port, password);
    if (cnt != 3) {
        printf("ERROR: invalid ez.cfg, format: <android_ip_addr> <ezApp_port> <ezApp_password>\n");
        exit(1);
    }

    // close ez.cfg
    fclose(fp);
}

void remove_leading_spaces(char *s)
{
    char *p = s;
    int   len;

    while (*p == ' ') p++;
    len = strlen(p);
    memcpy(s, p, len+1);
}

void put_fmt(FILE *fp, char *fmt, ...)
{
    va_list ap;
    int rc;

    va_start(ap, fmt);

    rc = vfprintf(fp, fmt, ap);
    if (rc < 0) {
        printf("ERROR: vfprintf failed\n");
        exit(1);
    }

    rc = fflush(fp);
    if (rc == EOF) {
        printf("ERROR: fflush failed\n");
        exit(1);
    }

    va_end(ap);
}

char *get_str(FILE *fp, char *s, int s_len)
{
    char *p;
    int len;

    s[0] = '\0';

    p = fgets(s, s_len, fp);
    if (p == NULL) {
        printf("ERROR: get failed\n");
        exit(1);
    }

    len = strlen(s);
    if (len > 0 && s[len-1] == '\n') {
        s[len-1] = '\0';
    }

    return s;
}

// -----------------  CONNECT TO ANDROID  -----------------------------------

void connect_to_android(void)
{
    int                ret;
    struct sockaddr_in addr;
    socklen_t          addrlen;
    char               response[100];

    // create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("ERROR: socket, %s\n", strerror(errno));
        exit(1);
    }

    // connect
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = inet_addr(ipaddr);
    addrlen = sizeof(addr);
    ret = connect(sockfd,  (struct sockaddr*)&addr, addrlen);
    if (ret != 0) {
        printf("ERROR: connect %s:%d, %s\n", ipaddr, port, strerror(errno));
        exit(1);
    }

    // create fp for socket fd
    sockfp = fdopen(sockfd, "w+");

    // send password, and get response
    put_fmt(sockfp, "%s\n", password);
    get_str(sockfp, response, sizeof(response));
    if (strcmp(response, "password okay") != 0) {
        printf("ERROR: password invalid\n");
        exit(1);   
    }

    // get the ezApp current working dir
    get_str(sockfp, cwd, sizeof(cwd));
    printf("cwd = '%s'\n", cwd);
}

// -----------------  RUN CMD ON ANDROID  -----------------------------------

void run_cmd_on_android(char *cmdline)
{
    char s[200];
    char *p;

    put_fmt(sockfp, "run %s\n", cmdline);

    while (true) {
        get_str(sockfp, s, sizeof(s));

        printf("%s\n", s);
        if ((p = strstr(s, "END_OF_DATA"))) {
            break;
        }
    }

#if 0
    char cmdline_buff[1000];
    int ret;

    // write 'r' to android
    // write cmdline to android
    memset(cmdline_buff, 0, sizeof(cmdline_buff));
    cmdline_buff[0] = 'r';
    cmdline_buff[1] = ' ';
    strcpy(&cmdline_buff[2], cmdline);

    ret = write(sockfd, cmdline_buff, sizeof(cmdline_buff));
    

    // read cmd exit_status and data_len from android

    // alloc and read data from android
#endif
}

#if 0
help
cd
pwd
cp file_path  android:file_path
cp android:file_path  file_path
cp http://xxx.yyy file_path

LATER
vi

ls rm mkdir curl, ....



#endif
