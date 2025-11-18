#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
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

#define PASSWORD_OKAY "password okay"

char *prompt = "ezsh> ";

char ipaddr[100];
int  port;
char password[100];

char cwd[100];

int sockfd;

void remove_leading_spaces(char *s);
void read_ez_cfg(void);
void read_working_dir(void);
void connect_to_android(void);
void run_r_type_cmd_on_android(char *cmdline, char **data, int *data_len);

// --------------------------------------------------------------------------

int main(int argc, char **argv)
{
    char *cmdline, *cmdline_copy;
    char *cmd, *args[20], *data=NULL;
    int   max_args, data_len;

    // read ez.cfg file
    read_ez_cfg();

    // connect to android, and validata password
    connect_to_android();

    // read working dir from android
    read_working_dir();
    printf("CWD '%s'\n", cwd);

    // runtime loop
    while (true) {
        // issue prompt, and read input line
        cmdline = readline(prompt);
        if (cmdline == NULL) {
            break;
        }
        printf("GOT '%s'\n", cmdline);

        // remove leading spaces
        remove_leading_spaces(cmdline);
        printf("REMOVED '%s'\n", cmdline);

        // if line is blank then continue
        if (cmdline[0] == '\0') {
            printf("blank\n");
            continue;
        }

        // make copy of input line, and tokenize
        cmdline_copy = strdup(cmdline);
        cmd = strtok(cmdline_copy, " ");
        max_args = 0;
        char *p;
        while ((p = strtok(NULL, " "))) {
            args[max_args++] = p;
        }

        printf("STRTOK cmd=%s max_args=%d - ", cmd, max_args);
        for (int i = 0; i < max_args; i++) printf(" '%s' ", args[i]);
        printf("\n");

        // process the input line

        run_r_type_cmd_on_android(cmdline, &data, &data_len);
        fwrite(data, 1, data_len, stdout);
        free(data);
        data = NULL;

        // free cmdline
        free(cmdline);
        free(cmdline_copy);
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

void read_working_dir(void)
{
    int ret;

    ret = read(sockfd, cwd, sizeof(cwd));
    if (ret != sizeof(cwd)) {
        printf("ERROR: failed to read ezApp working dir\n");
        exit(1);
    }
}

// -----------------  CONNECT TO ANDROID  -----------------------------------

void connect_to_android(void)
{
    int                ret;
    struct sockaddr_in addr;
    socklen_t          addrlen;
    char               password_response[100];

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

    // send password to android
    ret = write(sockfd, password, sizeof(password));
    printf("PASSWD SEND ret %d\n", ret);
    if (ret != sizeof(password)) {
        printf("ERROR: invalid password\n");
        exit(1);
    }

    // read password validation response from android
    ret = read(sockfd, password_response, sizeof(password_response));
    printf("PASSWD RESP = '%s'\n", password_response);
    if (ret != sizeof(password_response) || strcmp(password_response, PASSWORD_OKAY) != 0) {
        printf("ERROR: invalid password\n");
        exit(1);
    }
}

void run_r_type_cmd_on_android(char *cmdline, char **data, int *data_len)
{
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
