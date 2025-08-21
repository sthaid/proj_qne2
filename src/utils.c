#include <std_hdrs.h>

#include <utils.h>

// ----------------- TIME --------------------

long util_microsec_timer(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC,&ts);
    return  ((long)ts.tv_sec * 1000000) + ((long)ts.tv_nsec / 1000);
}

long util_get_real_time_microsec(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME,&ts);
    return ((long)ts.tv_sec * 1000000) + ((long)ts.tv_nsec / 1000);
}

char *util_time2str(char * str, long us, int gmt, int display_ms, int display_date)
{
    struct tm tm;
    time_t secs;
    int cnt;
    char * s = str;

    secs = us / 1000000;

    if (gmt) {
        gmtime_r(&secs, &tm);
    } else {
        localtime_r(&secs, &tm);
    }

    if (display_date) {
        cnt = sprintf(s, "%02d/%02d/%02d ",
                         tm.tm_mon+1, tm.tm_mday, tm.tm_year%100);
        s += cnt;
    }

    cnt = sprintf(s, "%02d:%02d:%02d",
                     tm.tm_hour, tm.tm_min, tm.tm_sec);
    s += cnt;

    if (display_ms) {
        cnt = sprintf(s, ".%03d", (int)((us % 1000000) / 1000));
        s += cnt;
    }

    if (gmt) {
        strcpy(s, " GMT");
    }

    return str;
}

// -----------------  FILE READ/WRITE  -----------------------

int util_write_file(char *path, void *buf, int len)
{
    int fd, ret;

    fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0666);
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

void *util_read_file(char *path, int *len_ret)
{
    int fd, ret;
    struct stat statbuf;
    void *buf;

    ret = stat(path, &statbuf);
    if (ret < 0) {
        return NULL;
    }

    buf = malloc(statbuf.st_size);
    if (buf == NULL) {
        return NULL;
    }

    fd = open(path, O_RDONLY);
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

// -----------------  GET / SET PARAMS  ----------------------

char *util_get_str_param(char *name, char *default_value);
void util_set_str_param(char *name, char *value);
int util_get_int_param(char *name, int default_value);
void util_set_int_param(char *name, int value);
void util_print_params(void);

#define MAX_PARAMS 32

static struct {
    char *name;
    char *value;
} params[MAX_PARAMS];
static int max_params;
static char params_dir[100];

static void remove_trailing_newline(char *s)
{
    int len = strlen(s);

    if (len > 0) {
        s[len-1] = '\0';
    }
}

static void read_params_file(void)
{
    char s[200], name[100];
    int cnt, n;
    FILE *fp;
    char current_dir[100];

    getcwd(current_dir, sizeof(current_dir));
    if (strcmp(current_dir, params_dir) == 0) {
        return;
    }
    strcpy(params_dir, current_dir);

    printf("INFO %s: reading params file in dir '%s'\n", __func__, current_dir);

    memset(params, 0, sizeof(params));
    max_params = 0;

    fp = fopen("params", "r");
    if (fp == NULL) {
        printf("INFO %s: params file does not exist\n", __func__);
        return;
    }

    while (fgets(s, sizeof(s), fp) != NULL) {
        remove_trailing_newline(s);
        n = 0;
        cnt = sscanf(s, "%s = %n", name, &n);
        if (cnt != 1 || n == 0) {
            printf("ERROR %s: read_params_file '%s'\n", __func__, s);
            fclose(fp);
            return;
        }

        params[max_params].name = strdup(name);
        params[max_params].value = strdup(s+n);
        max_params++;
    }

    fclose(fp);

    printf("INFO %s: max_params=%d\n", __func__, max_params);
    for (int i = 0; i < max_params; i++) {
        printf("INFO %s:   %s = %s\n", __func__, params[i].name, params[i].value);
    }
}

static void write_params_file(void)
{
    FILE *fp;
    char current_dir[100];

    getcwd(current_dir, sizeof(current_dir));
    if (strcmp(current_dir, params_dir) != 0) {
        printf("ERROR %s: write_params_file, current_dir=%s params_dir=%s\n",
               __func__, current_dir, params_dir);
        return;
    }

    printf("INFO %s: writing params file in dir '%s'\n", __func__, current_dir);
    printf("INFO %s: max_params=%d\n", __func__, max_params);
    for (int i = 0; i < max_params; i++) {
        printf("INFO %s:   %s = %s\n", __func__, params[i].name, params[i].value);
    }

    fp = fopen("params", "w");
    if (fp == NULL) {
        printf("ERROR %s: write_params_file, fopen failed, %s\n", __func__, strerror(errno));
        return;
    }

    for (int i = 0; i < max_params; i++) {
        fprintf(fp, "%-16s = %s\n", params[i].name, params[i].value);
    }

    fclose(fp);
}

char *util_get_str_param(char *name, char *default_value)
{
    int i;

    // if haven't read the params file then do so
    read_params_file();

    // search for matching name
    for (i = 0; i < max_params; i++) {
        if (strcmp(name, params[i].name) == 0) {
            break;
        }
    }

    // if found then 
    //   return value
    // else
    //   add param, set to default value, and write file
    // endif
    if (i < max_params) {
        return params[i].value;
    } else {
        if (max_params >= MAX_PARAMS) {
            printf("ERROR %s: params tbl is full\n", __func__);
            return default_value;
        }
        params[max_params].name = strdup(name);
        params[max_params].value = strdup(default_value);
        max_params++;
        write_params_file();
        return default_value;
    }
}

void util_set_str_param(char *name, char *value)
{
    int i;

    // if haven't read the params file then do so
    read_params_file();

    // search for matching name
    for (i = 0; i < max_params; i++) {
        if (strcmp(name, params[i].name) == 0) {
            break;
        }
    }

    // if found then
    //   replace its value
    // else
    //   add param to the end
    // endif
    if (i < max_params) {
        free(params[i].value);
        params[i].value = strdup(value);
    } else {
        if (max_params >= MAX_PARAMS) {
            printf("ERROR %s: params tbl is full\n", __func__);
            return;
        }
        params[max_params].name = strdup(name);
        params[max_params].value = strdup(value);
        max_params++;
    }

    // write the params file
    write_params_file();
}

int util_get_int_param(char *name, int dflt_val)
{
    char  dflt_val_str[20];
    char *value_str;
    int   value_int;
    int   cnt;

    // create the default value string, and
    // call util_get_str_param to get the value_str
    sprintf(dflt_val_str, "%d", dflt_val);
    value_str = util_get_str_param(name, dflt_val_str);

    // convert value_str, returned by util_get_str_param, to value_int
    cnt = sscanf(value_str, "%d", &value_int);

    // the conversion can fail if value_str is not an integer,
    // if the conversion fails then call util_set_int_param, and 
    // return the default value
    if (cnt != 1) {
        util_set_int_param(name, dflt_val);
        return dflt_val;
    }

    // return the integer param value
    return value_int;
}

void util_set_int_param(char *name, int value)
{
    char value_str[20];

    // create value string, and
    // call util_set_str_param to set it
    sprintf(value_str, "%d", value);
    util_set_str_param(name, value_str);
}    

void util_print_params(void)
{
    int i;

    read_params_file();

    printf("INFO %s: max_params=%d\n", __func__, max_params);
    for (i = 0; i < max_params; i++) {
        printf("INFO %s:   %s = %s\n", __func__, params[i].name, params[i].value);
    }
}

// -----------------  NETWORK  -------------------------------

char *util_get_ipaddr(void)
{
    static char ipaddr[20];
    int rc, a=0, b=0, c=0, d=0;
    unsigned int addr;
    struct ifaddrs *ifap, *ifap_orig;;

    strcpy(ipaddr, "xxx.xxx.xxx.xxx");

    rc = getifaddrs(&ifap_orig);
    if (rc != 0) {
        printf("ERROR %s: getifaddrs, %s\n", __func__, strerror(errno));
        return ipaddr;
    }

    ifap = ifap_orig;
    while (ifap) {
        //printf("ifa_name = %s\n", ifap->ifa_name);
        if (ifap->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *x = (struct sockaddr_in*)ifap->ifa_addr;

            addr = htonl(x->sin_addr.s_addr);

            if (((addr >> 24) & 0xff) == 127) {
                goto next;
            }

            a = (addr >> 24) & 0xff;
            b = (addr >> 16) & 0xff;
            c = (addr >>  8) & 0xff;
            d = (addr >>  0) & 0xff;

            if (a == 192 || a == 10) {
                sprintf(ipaddr, "%d.%d.%d.%d", a,b,c,d);
                break;
            }
        }
            
next:
        ifap = ifap->ifa_next;
    }

    freeifaddrs(ifap_orig);

    if (ipaddr[0] == 'x' && a != 0) {
        sprintf(ipaddr, "%d.%d.%d.%d", a,b,c,d);
    }

    return ipaddr;

#if 0
    // The following approach doesn't work on Android, Google AI says:
    //
    // "The ip program, along with other network utilities like ifconfig, 
    //  route, and netstat, cannot be run directly by non-privileged user 
    //  applications on Android due to security restrictions implemented by 
    //  the operating system.:

    static char ipaddr[20];
    FILE       *fp;
    char        s[100], s1[100];
    int         a, b, c, d;

    strcpy(ipaddr, "xxx.xxx.xxx.xxx");

    fp = popen("ip -4 addr show | grep \" inet \"", "r");
    if (fp != NULL) {
        while (fgets(s, sizeof(s), fp) != NULL) {
            if (sscanf(s, "%s %d.%d.%d.%d", s1, &a, &b, &c, &d) == 5 &&
                strcmp(s1, "inet") == 0 &&
                a != 127)
            {
                sprintf(ipaddr, "%d.%d.%d.%d", a, b, c, d);
                break;
            }
        }

        fclose(fp);
    } else {
        printf("ERROR %s: popen failed, %s\n", __func__, strerror(errno));
    }

    return ipaddr;
#endif
}

