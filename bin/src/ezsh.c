#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>

#include <readline/readline.h>
#include <readline/history.h>

// xxx todo
// - vi cmd
// - test WEXITSTATUS on android

// NOTES:
// - status returns xxx

//
// defines
//

#define NOT_A_SPECIAL_CMD -9999

#define MAX_ALIAS 100

//
// typedefs
//

typedef struct {
    char *cmd;
    char *alias;
} alias_t;

//
// variables
//

// these are obtained from the ez.cfg file
char    ipaddr[200];
int     port;
char    password[200];
alias_t alias_tbl[MAX_ALIAS];
int     max_alias;

// current working directory
char    cwd[200];
char    cwd_initial[200];

// fp used to communicate to server process running on android
FILE   *sockfp;

//
// prototypes
//

// main
void read_ez_cfg(void);
void connect_to_android(void);
void substitue_alias(char *cmdline);
int run_cmd(char *cmdline);

// run special cmd
int run_special_cmd(char *cmdline);
int special_cmd_copy_file_to_android(char *src_path, char *dest_dir);
int special_cmd_copy_file_from_android(char *src_path, char *dest_path);
int special_cmd_cd(char *path);
int special_cmd_pwd(void);
int special_cmd_alias(void);

// run cmd on android
int run_cmd_on_android(char *cmdline, char *data_out, int data_out_len, char **data_in, int *data_in_len);

// utils
void sanitize(char *s);
void put_fmt(FILE *fp, char *fmt, ...);
char *get_str(FILE *fp, char *s, int s_len);
int read_file(char *fn, void **buf, int *buf_len);
int write_file(char *fn, void *buf, int len);

// -----------------  MAIN  -------------------------------------------------

int main(int argc, char **argv)
{
    char *cmdline;
    int   status;

    // validate args, print usage
    if (argc > 2) {
        printf("Usage: ezsh [<cmdline>]\n");
        // xxx more details
        exit(1);
    }

    // read ez.cfg file, to get ipaddr, port, password, and cmd aliases
    read_ez_cfg();

    // connect to android: also validates password and gets curr-working-dir (cwd)
    connect_to_android();

    // if argv[1] provided then use it for cmdline, and then end program
    if (argc == 2) {
        status = run_cmd(argv[1]);
        return status != 0 ? 1 : 0;
    }

    // runtime loop
    while (true) {
        // construct prompt
        char prompt[200], temp[200];
        strcpy(temp, cwd);
        snprintf(prompt, sizeof(prompt), "ezsh %s> ", basename(temp));

        // read cmdline
        // - if NULL then end program
        // - remove leading and trailing spaces and trailing newline
        // - if blank line then continue
        // - if 'q' then end program
        // - add cmdline to history
        cmdline = readline(prompt);
        if (cmdline == NULL) {
            break;
        }
        sanitize(cmdline);
        if (cmdline[0] == '\0') {
            continue;
        }
        if (strcmp(cmdline, "q") == 0) {
            break;
        }
        add_history(cmdline);

        // substitue alias
        substitue_alias(cmdline);

        // process the cmdline
        run_cmd(cmdline);

        // free cmdline
        free(cmdline);
    }
}

void read_ez_cfg(void)
{
    char  self_path[200], ez_cfg_path[200], *self_dir, s[200];
    FILE *fp;
    int   line_num=0;

    // get path to ez.cfg file
    readlink("/proc/self/exe", self_path, sizeof(self_path));
    self_dir = dirname(self_path);
    sprintf(ez_cfg_path, "%s/ez.cfg", self_dir);

    // open ez.cfg file
    fp = fopen(ez_cfg_path, "r");
    if (fp == NULL) {
        printf("ERROR: failed to open %s\n", ez_cfg_path);
        exit(1);
    }

    // read and process lines from ez.cfg file
    while (fgets(s, sizeof(s), fp) != NULL) {
        line_num++;

        // remove leading and trailing spaces and trailing newline
        sanitize(s);

        // skip blank lines and lines begining with '#'
        if (s[0] == '\0' || s[0] == '#') {
            continue;
        }

        // tokenize line
        char *id = strtok(s, " ");
        char *val = strtok(NULL, " ");
        char *rest = strtok(NULL, "");

        // must have at least id and val
        if (id == NULL || val == NULL) {
            continue;
        }

        // parse the line
        if (strcmp(id, "ipaddr") == 0) {
            strcpy(ipaddr, val);
        } else if (strcmp(id, "port") == 0) {
            sscanf(val, "%d", &port);
        } else if (strcmp(id, "password") == 0) {
            strcpy(password, val);
        } else if (strcmp(id, "alias") == 0) {
            if (max_alias == MAX_ALIAS) {
                printf("ERROR: alias tbl is full, ez.cfg line %d\n", line_num);
                continue;
            }
            if (rest == NULL) {
                printf("ERROR: invalid alias, ez.cfg line %d\n", line_num);
                continue;
            }
            sanitize(rest);
            alias_tbl[max_alias].cmd = strdup(val);
            alias_tbl[max_alias].alias = strdup(rest);
            max_alias++;
        } 
    }

    // close fp
    fclose(fp);

    // verify ipaddr, port and password are set
    if (ipaddr[0] == '\0') {
        printf("ERROR: ippadr needed in ez.cfg\n");
        exit(1);
    }
    if (port == 0) {
        printf("ERROR: port needed in ez.cfg\n");
        exit(1);
    }
    if (password[0] == '\0') {
        printf("ERROR: password needed in ez.cfg\n");
        exit(1);
    }
}

void connect_to_android(void)
{
    int                ret, len, sockfd;
    struct sockaddr_in addr;
    socklen_t          addrlen;
    char               response[200];

    // create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("ERROR: socket\n");
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

    // ensure cwd includes terminating '/'
    len = strlen(cwd);
    if (len == 0) {
        strcpy(cwd, "/");
    } else if (cwd[len-1] != '/') {
        strcat(cwd, "/");
    }

    // save initial cwd
    strcpy(cwd_initial, cwd);
}

void substitue_alias(char *cmdline)
{
    char *p, temp[1000];
    int   i;

    p = strchr(cmdline, ' ');
    if (p) *p = '\0';

    for (i = 0; i < max_alias; i++) {
        if (strcmp(cmdline, alias_tbl[i].cmd) == 0) {
            if (p) *p = ' ';
            strcpy(temp, cmdline+strlen(alias_tbl[i].cmd));
            strcpy(cmdline, alias_tbl[i].alias);
            strcat(cmdline, temp);
            return;
        }
    }

    if (p) *p = ' ';
}

int run_cmd(char *cmdline)
{
    int status;
    char cd_plus_cmdline[1000];

    status = run_special_cmd(cmdline);

    if (status == NOT_A_SPECIAL_CMD) {
        sprintf(cd_plus_cmdline, "cd %s; %s", cwd, cmdline);
        status = run_cmd_on_android(cd_plus_cmdline, NULL, 0, NULL, 0);
    }

    return status;
}

// -----------------  RUN SPECIAL CMD  ----------------------------------

int run_special_cmd(char *cmdline)
{
    char cmd[200], arg1[200], arg2[200];

    // extract cmd, arg1, and arg2 from cmdline
    cmd[0] = arg1[0] = arg2[0] = '\0';
    sscanf(cmdline, "%s %s %s", cmd, arg1, arg2);
    if (cmd[0] == '\0') {
        printf("ERROR: run_special_cmd cmdline arg invalid\n");
        exit(1);   
    }

    // process special cmds
    if (strcmp(cmd, "cd") == 0) {
        return special_cmd_cd(arg1);
    } else if (strcmp(cmd, "pwd") == 0) {
        return special_cmd_pwd();
    } else if (strcmp(cmd, "alias") == 0) {
        return special_cmd_alias();
    } else if (strcmp(cmd, "put") == 0) {
        // xxx move the complextiy to the routines
        char *src_path, dest_path[400], temp[200];
        char *src_file_name;

        // init develsys src_path
        src_path = arg1;
        if (src_path[0] == '\0') {
            printf("ERROR: develsys src_path required\n");
            return -EINVAL;
        }

        // init android dest_path
        // xxx could send the filename too, and let android fix up the dest_path
        //     if the dest_path is a directory
        if (arg2[0] == '\0') {
            strcpy(temp, src_path);
            src_file_name = basename(temp);
            sprintf(dest_path, "%s%s", cwd, src_file_name);
        } else if (arg2[0] == '/') {
            strcpy(dest_path, arg2);
        } else {
            sprintf(dest_path, "%s%s", cwd, arg2);
        }
        
        return special_cmd_copy_file_to_android(src_path, dest_path);
    } else if (strcmp(cmd, "get") == 0) {
        char src_path[400], dest_path[200], temp[200];
        char *src_file_name;

        // init android src_path
        if (arg1[0] == '\0') {
            printf("ERROR: android src_path required\n");
            return -EINVAL;
        } else if (arg1[0] == '/') {
            strcpy(src_path, arg1);
        } else {
            sprintf(src_path, "%s%s", cwd, arg1);
        }

        // init devlsys dest_path
        // xxx also handle the case where arg2 is a directory
        if (arg2[0] == '\0') {
            strcpy(temp, src_path);
            src_file_name = basename(temp);
            strcpy(dest_path, src_file_name);
        } else {
            strcpy(dest_path, arg2);
        }

        return special_cmd_copy_file_from_android(src_path, dest_path);
    } else if (strcmp(cmd, "vi") == 0) {
        // xxx todo
        printf("ERROR: vi cmd not supported yet\n");
        return -EINVAL;
    } else {
        // not a special cmd
        return NOT_A_SPECIAL_CMD;
    }

    // not reached
}

int special_cmd_copy_file_to_android(char *src_path, char *dest_path)
{
    char  cmdline[1000];
    void *data;
    int   data_len;
    int   status;

    //printf("put %s %s\n", src_path, dest_path);

    // read src_path file
    status = read_file(src_path, &data, &data_len);
    if (status != 0) {
        printf("ERROR: read_file %s, %s\n", src_path, strerror(errno));
        free(data);
        return -errno;
    }

    // run 'put' on android
    sprintf(cmdline, "put %s", dest_path);
    status = run_cmd_on_android(cmdline, data, data_len, NULL, 0);

    // free data
    free(data);

    // return status
    return status;
}

int special_cmd_copy_file_from_android(char *src_path, char *dest_path)
{
    char  cmd[1000];
    char *data = NULL;
    int   data_len = 0, status;

    //printf("get %s %s\n", src_path, dest_path);

    // run 'get' on android
    sprintf(cmd, "get %s", src_path);
    status = run_cmd_on_android(cmd, NULL, 0, &data, &data_len);
    if (status != 0) {
        printf("ERROR: file data not recvd from android\n");
        free(data);
        return status;
    }

    // write file to dest_dir on develsys
    status = write_file(dest_path, data, data_len);
    if (status != 0) {
        printf("ERROR: failed to create '%s', %s\n", dest_path, strerror(-status));
    }

    // free data
    free(data);

    // return status
    return status;
}

// This routine updates cwd; and will always terminate cwd with '/'.
int special_cmd_cd(char *path)
{
    int   len;
    char  new_cwd[200];
    char *token;

    // sanity check that cwd begins and ends with '/'
    len = strlen(cwd);
    if (cwd[0] != '/' || cwd[len-1] != '/') {
        printf("ERROR: invalid cwd '%s'\n", cwd);
        exit(1);
    }

    // use new_cwd as work area, until it is validated
    strcpy(new_cwd, cwd);

    // update new_cwd, using the supplied path
    if (path[0] == '\0') {
        strcpy(new_cwd, cwd_initial);
    } else if (path[0] == '/') {
        strcpy(new_cwd, path);
    } else {
        // ensure path terminates with '/'
        len = strlen(path);
        if (path[len-1] != '/') {
            strcat(path, "/");
        }

        // tokenize path using '/' separator;
        // and update new_cwd using the value of each token
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

    // ensure new_cwd terminates with '/'
    len = strlen(new_cwd);
    if (len > 0 && new_cwd[len-1] != '/') {
        strcat(new_cwd, "/");
    }

    // run_cmd_on_android to verify new_cwd is an android directory;
    // if so then the new_cwd will be used
    char cmd[300];
    int  status;
    sprintf(cmd, "if [ ! -d %s ]; then exit 1; else exit 0; fi", new_cwd);
    status = run_cmd_on_android(cmd, NULL, 0, NULL, 0);
    if (status == 0) {
        strcpy(cwd, new_cwd);
    }

    // return status
    return status;
}

int special_cmd_pwd(void)
{
    printf("%s\n", cwd);
    return 0;
}

int special_cmd_alias(void)
{
    int i;

    for (i = 0; i < max_alias; i++) {
        printf("%-16s %s\n", alias_tbl[i].cmd, alias_tbl[i].alias);
    }
    return 0;
}

// -----------------  RUN CMD ON ANDROID  -----------------------------------

int run_cmd_on_android(char *cmdline, char *data_out, int data_out_len,
                       char **data_in, int *data_in_len)
{
    char  s[200];
    char *p;
    int   rc;
    int   status = 99;

    // send 'run' and cmdline to android
    put_fmt(sockfp, "run\n");
    put_fmt(sockfp, "%s\n", cmdline);

    // if data_out is provided then send data buffer to android 
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

    // if data_in is provided then recv data buffer from adnroid
    } else if (data_in != NULL) {
        char *buf;
        int   buf_len;

        // get buf_len from android
        get_str(sockfp, s, sizeof(s));
        if (sscanf(s, "data_len %d", &buf_len) != 1) {
            printf("ERROR: did not recv data_len\n");
            exit(1);
        }

        // allocate buf
        buf = calloc(buf_len, 1);

        // read data from android
        rc = fread(buf, 1, buf_len, sockfp);
        if (rc != buf_len) {
            printf("ERROR: fread failed, %s\n", strerror(errno));
            free(buf);
            exit(1);
        }

        // read cmd completion string from android
        get_str(sockfp, s, sizeof(s));
        if (strncmp(s, "CMD_COMPLETE ", 13) != 0) {
            printf("ERROR: did not recv CMD_COMPLETE\n");
            free(buf);
            exit(1);
        }

        // return data to caller; caller must free data
        *data_in = buf;
        *data_in_len = buf_len;

        // parse the status returned from android
        sscanf(s+13, "%d", &status);

    // otherwise android will have run this cmd using popen;
    // read and print the provided output from popen
    } else {
        while (true) {
            // get string from android, and print
            get_str(sockfp, s, sizeof(s));

            // check for cmd complete, and parse status
            if ((p = strstr(s, "CMD_COMPLETE "))) {
                sscanf(p+13, "%d", &status);
                *p = '\0';
                if (strlen(s) > 0) {
                    printf("%s\n", s);
                }
                break;
            }

            printf("%s\n", s);
        }
    }

    // print status
    // - status == 0 : success
    // - status > 0  : is an exitcode from the cmdline executed on android
    // - status < 0  : is an errno
    if (status > 0) {
        printf("ERROR: exit_status %d\n", status);
    } else if (status < 0) {
        printf("ERROR: %s\n", strerror(-status));
    }

    // return status
    return status;
}

// -----------------  UTILS  ----------------------------------------

void sanitize(char *s)
{
    char *p = s;
    int   len;

    // return if s is NULL
    if (s == NULL) {
        return;
    }

    // remove leading spaces
    while (*p == ' ') {
        p++;
    }
    len = strlen(p);
    memmove(s, p, len+1);

    // if length of s is 0 then return
    if (len == 0) {
        return;
    }

    // remove trailing spaces and newline chars
    p = &s[len-1];
    while (p >= s && (*p == ' ' || *p == '\n')) {
        *p = '\0';
        p--;
    }
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

int read_file(char *fn, void **buf_arg, int *buf_len_arg)
{
    int fd, ret;
    struct stat statbuf;
    char *buf;

    *buf_len_arg = 0;
    *buf_arg = NULL;

    ret = stat(fn, &statbuf);
    if (ret < 0) {
        return errno ? -errno : -EINVAL;
    }
    
    fd = open(fn, O_RDONLY);
    if (fd < 0) {
        return errno ? -errno : -EINVAL;
    }

    buf = malloc(statbuf.st_size);
    if (buf == NULL) {
        close(fd);
        return errno ? -errno : -EINVAL;
    }
    
    ret = read(fd, buf, statbuf.st_size);
    if (ret != statbuf.st_size) {
        free(buf);
        close(fd);
        return errno ? -errno : -EINVAL;
    }

    close(fd);

    *buf_arg = buf;
    *buf_len_arg = statbuf.st_size;
    return 0;  
}

int write_file(char *fn, void *buf, int len)
{
    int fd, ret;

    fd = open(fn, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    if (fd < 0) {
        return errno ? -errno : -EINVAL;
    }

    ret = write(fd, buf, len);
    if (ret != len) {
        return errno ? -errno : -EINVAL;
    }

    close(fd);
    return 0;
}

