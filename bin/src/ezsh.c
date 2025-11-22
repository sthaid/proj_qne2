#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>  // xxx all needed?
#include <sys/stat.h>  // xxx all needed?
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>

#include <readline/readline.h>
#include <readline/history.h>

// info from ez.cfg
char ipaddr[100];
int  port;
char password[100];


char cwd[100];
char cwd_initial[100];
int sockfd;  // private
FILE *sockfp;

// prototypes
void remove_leading_spaces(char *s);
void read_ez_cfg(void);
void connect_to_android(void);

int run_special_cmd(char *cmdline);

int run_cmd_on_android(char *cmdline, char *data_out, int data_out_len, char **data_in, int *data_in_len);

// xxx todo
// - status and errno returns
// - cpta cpfa cmds
// - q cmd
// - include alias in .ezrc file, also ipconfig & password
//    - ls alias
// - END_OF_DATA marker
// - copy file to/from android
// - setting for password, require it be changed , use change_me
// - vi cmd
// - clean up code and comments
// -  use strings of either size 100 or 1000

// xxx done
// - hist
// - cd and pwd cmds
//   - prepend the current cd value

// -----------------  MAIN  -------------------------------------------------

int main(int argc, char **argv)
{
    char *cmdline;
    int   rc;

    // read ez.cfg file, to get ipaddr, port, and password
    read_ez_cfg();

    // connect to android: also validates password and gets curr-working-dir
    connect_to_android();

    // runtime loop
    while (true) {
        // read cmdline
        char prompt[100], cwd_copy[100];
        strcpy(cwd_copy, cwd);
        snprintf(prompt, sizeof(prompt), "ezsh %s> ", basename(cwd_copy));

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
        rc = run_special_cmd(cmdline);
        if (rc == -1) {
            char cmdline2[200];
            sprintf(cmdline2, "cd %s; %s", cwd, cmdline);
            run_cmd_on_android(cmdline2, NULL, 0, NULL, 0);
        }

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
    int                ret, len;
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
    len = strlen(cwd);
    if (len == 0) {
        strcpy(cwd, "/");
    } else if (cwd[len-1] != '/') {
        strcat(cwd, "/");
    }
    printf("cwd updated = '%s'\n", cwd);

    strcpy(cwd_initial, cwd);
}

// -----------------  RUN SPECIAL CMD  --------------------------------------

void proc_cd(char *path);
void proc_copy_file_to_android(char *src_path, char *dest_dir);
void proc_copy_file_from_android(char *src_path, char *dest_path);
void *read_file(char *fn, int *len_ret);
int write_file(char *fn, void *buf, int len);

int run_special_cmd(char *cmdline)
{
    char cmd[100], arg1[100], arg2[100];

    cmd[0] = arg1[0] = arg2[0] = '\0';
    sscanf(cmdline, "%s %s %s", cmd, arg1, arg2);
    if (cmd[0] == '\0') {
        printf("ERROR: run_special_cmd cmdline arg invalid\n");
        exit(1);   
    }
    printf("cmd=%s arg1=%s arg2=%s\n", cmd, arg1, arg2);

    if (strcmp(cmd, "cd") == 0) {
        proc_cd(arg1);
    } else if (strcmp(cmd, "pwd") == 0) {
        printf("%s\n", cwd);
    } else if (strcmp(cmd, "cpta") == 0) {
        char *src_path, dest_path[200], temp[200];
        char *src_file_name;

        // init develsys src_path
        src_path = arg1;
        if (src_path[0] == '\0') {
            printf("ERROR: develsys src_path required\n");
            return 0;
        }

        // init android dest_path
        if (arg2[0] == '\0') {
            strcpy(temp, src_path);
            src_file_name = basename(temp);
            sprintf(dest_path, "%s%s", cwd, src_file_name);
        } else if (arg2[0] == '/') {
            strcpy(dest_path, arg2);
        } else {
            sprintf(dest_path, "%s%s", cwd, arg2);
        }
        
        proc_copy_file_to_android(src_path, dest_path);
    } else if (strcmp(cmd, "cpfa") == 0) {
        char src_path[200], dest_path[200], temp[200];
        char *src_file_name;

        // init android src_path
        if (arg1[0] == '\0') {
            printf("ERROR: android src_path required\n");
            return 0;
        } else if (arg1[0] == '/') {
            strcpy(src_path, arg1);
        } else {
            sprintf(src_path, "%s%s", cwd, arg1);
        }

        // init devlsys dest_path
        if (arg2[0] == '\0') {
            strcpy(temp, src_path);
            src_file_name = basename(temp);
            strcpy(dest_path, src_file_name);
        } else {
            strcpy(dest_path, arg2);
        }

        proc_copy_file_from_android(src_path, dest_path);
    } else if (strcmp(cmd, "vi") == 0) {
        // xxx toto
    } else {
        printf("xxx run_special returning -1\n");
        return -1;
    }

    return 0;
}

void proc_copy_file_to_android(char *src_path, char *dest_path)
{
    char  cmdline[1000];
    char *data;
    int   data_len;

    printf("cpta %s %s\n", src_path, dest_path);

    // read src_path file
    data = read_file(src_path, &data_len);
    if (data == NULL) {
        printf("ERROR: read_file %s, %s\n", src_path, strerror(errno));
        return;
    }

    // run 'cpta' on android
    sprintf(cmdline, "cpta %s", dest_path);
    run_cmd_on_android(cmdline, data, data_len, NULL, 0);

    // free data
    free(data);
}

void proc_copy_file_from_android(char *src_path, char *dest_path)
{
    char  cmd[1000];
    char *data = NULL;
    int   data_len = 0;

    printf("cpfa %s %s\n", src_path, dest_path);

    // run 'cpfa' on android
    sprintf(cmd, "cpfa %s", src_path);
    run_cmd_on_android(cmd, NULL, 0, &data, &data_len);
    if (data == NULL) {
        printf("ERROR: file data not recvd from android\n");
        return;
    }

    // write file to dest_dir on develsys
    write_file(dest_path, data, data_len);
    // xxx print error

    // free data
    free(data);
}

// xxx cwd must always end in '/'
void proc_cd(char *path)
{
    int len;
    char new_cwd[100];
    char *token;

    len = strlen(cwd);
    if (cwd[0] != '/' || cwd[len-1] != '/') {
        printf("ERROR: invalid cwd '%s'\n", cwd);
        exit(1);
    }

    strcpy(new_cwd, cwd);

    if (path[0] == '\0') {
        strcpy(new_cwd, cwd_initial);
    } else if (path[0] == '/') {
        strcpy(new_cwd, path);
    } else {
        len = strlen(path);
        if (path[len-1] != '/') {
            strcat(path, "/");
        }

        while ((token = strtok(path, "/"))) {
            path = NULL;
            if (strcmp(token, ".") == 0) {
                // do nothing
            } else if (strcmp(token, "..") == 0) {
                if (strcmp(new_cwd, "/") != 0) {
                    int idx = strlen(new_cwd) - 2;
                    while (new_cwd[idx] != '/') idx--;
                    new_cwd[idx+1] = '\0';
                }
            } else {
                strcat(new_cwd, token);
                strcat(new_cwd, "/");
            }
        }
    }

    // xxx validate new_cwd
    len = strlen(new_cwd);
    if (len > 0 && new_cwd[len-1] != '/') {
        printf("ADDING TERM slash\n");
        strcat(new_cwd, "/");
    }

    char cmd[200];
    sprintf(cmd, "dir_exists %s", new_cwd);
    int status = run_cmd_on_android(cmd, NULL, 0, NULL, 0);
    printf("GOT STATUS %d %s\n", status, (status ? strerror(status) : ""));

    // update cwd
    if (status == 0) {
        strcpy(cwd, new_cwd);
        printf("new cwd = %s\n", cwd);
    }
}

void *read_file(char *fn, int *len_ret)
{
    int fd, ret;
    struct stat statbuf;
    char *buf;

    ret = stat(fn, &statbuf);
    if (ret < 0) {
        return NULL;
    }
    
    buf = malloc(statbuf.st_size);
    if (buf == NULL) {
        return NULL;
    }
    
    fd = open(fn, O_RDONLY);
    if (fd < 0) {
        free(buf);
        return NULL;
    }

    ret = read(fd, buf, statbuf.st_size);
    if (ret != statbuf.st_size) {
        free(buf);
        return NULL;
    }

    close(fd);

    *len_ret = statbuf.st_size;
    return buf;
}

int write_file(char *fn, void *buf, int len)
{
    int fd, ret;

    printf("WRITE '%s'\n", fn);

    fd = open(fn, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    if (fd < 0) {
        return -1;
    }

    ret = write(fd, buf, len);
    if (ret != len) {
        return -1;
    }

    close(fd);
    return 0;
}


// -----------------  RUN CMD ON ANDROID  -----------------------------------

// xxx
// - send and recv binary data
// - status and errno returns

int run_cmd_on_android(char *cmdline, char *data_out, int data_out_len,
                       char **data_in, int *data_in_len)
{
    char s[200];
    char *p;
    int rc;
    int status = 99;

    printf("RUN_CMD_ON_ANDROID: '%s'\n", cmdline);

    put_fmt(sockfp, "run\n");
    put_fmt(sockfp, "%s\n", cmdline);

    if (data_out != NULL) {
        put_fmt(sockfp, "data_len %d\n", data_out_len);

        rc = fwrite(data_out, 1, data_out_len, sockfp);
        if (rc != data_out_len) {
            printf("ERROR: fwrite failed, %s\n", strerror(errno));
            exit(1);
        }

        get_str(sockfp, s, sizeof(s));
        if (strncmp(s, "CMD_COMPLETE ", 13) != 0) {
            printf("ERROR: did not recv CMD_COMPLETE\n");
            exit(1);
        }

        sscanf(s+13, "%d", &status);
        printf("got status %d\n", status);
    } else if (data_in != NULL) {
        char *buf;
        int   buf_len;

        get_str(sockfp, s, sizeof(s));
        if (sscanf(s, "data_len %d", &buf_len) != 1) {
            printf("ERROR: did not recv data_len\n");
            exit(1);
        }

        buf = calloc(buf_len, 1);

        rc = fread(buf, 1, buf_len, sockfp);
        if (rc != buf_len) {
            printf("ERROR: fread failed, %s\n", strerror(errno));
            free(buf);
            exit(1);
        }

        get_str(sockfp, s, sizeof(s));
        if (strncmp(s, "CMD_COMPLETE ", 13) != 0) {
            printf("ERROR: did not recv CMD_COMPLETE\n");
            free(buf);
            exit(1);
        }

        // caller must free data
        *data_in = buf;
        *data_in_len = buf_len;


        sscanf(s+13, "%d", &status);
        printf("got status %d\n", status);
    } else {
        while (true) {
            get_str(sockfp, s, sizeof(s));

            printf("%s\n", s);
            if ((p = strstr(s, "CMD_COMPLETE "))) {
                sscanf(p+13, "%d", &status);
                printf("got status %d\n", status);
                break;
            }
        }
    }

    return status;
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
