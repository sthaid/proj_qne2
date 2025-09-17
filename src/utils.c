#include <std_hdrs.h>

#include <utils.h>
#include <logging.h>

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

int util_write_file(char *dir, char *fn, void *buf, int len)
{
    int fd, ret;
    char path[200];

    sprintf(path, "%s/%s", dir, fn);

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

// xxx comment on extra byte
void *util_read_file(char *dir, char *fn, int *len_ret)
{
    int fd, ret;
    struct stat statbuf;
    char *buf;
    char path[200];

    sprintf(path, "%s/%s", dir, fn);

    ret = stat(path, &statbuf);
    if (ret < 0) {
        return NULL;
    }

    buf = malloc(statbuf.st_size+1);
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

    buf[statbuf.st_size] = '\0';

    close(fd);

    *len_ret = statbuf.st_size;
    return buf;
}

// -----------------  FILE MAP -------------------------------

#define PAGE_SIZE        0x4000  // 16k
#define MAX_FILE_MAP_TBL 20

static pthread_mutex_t file_map_mutex = PTHREAD_MUTEX_INITIALIZER;

static struct file_map_tbl_s {
    void *addr;
    int   len;
    ino_t ino;
    int   refcnt;
} file_map_tbl[MAX_FILE_MAP_TBL];

void *util_map_file(char *dir, char *file, int len, bool create_if_needed)
{
    char   path[100];
    int    tbl_idx, i, fd, rc;
    int    file_exists, file_len, file_inode;
    void  *addr = NULL;
    struct stat statbuf;

    // lock mutex
    pthread_mutex_lock(&file_map_mutex);

    // round len up to multiple of pagesize
    if (len & (PAGE_SIZE-1)) {
        len = (len + PAGE_SIZE) & ~(PAGE_SIZE-1);
    }

    // find free entry in file_map_tbl, if not found then return error
    for (tbl_idx = 0; tbl_idx < MAX_FILE_MAP_TBL; tbl_idx++) {
        if (file_map_tbl[tbl_idx].addr != NULL) {
            break;
        }
    }
    if (tbl_idx == MAX_FILE_MAP_TBL) {
        ERROR("no free tbl entries\n");
        goto done;  
    }

    // stat the file to determine if it exists, and its length and inode number
    sprintf(path, "%s/%s", dir, file);
    rc = stat(path, &statbuf);
    file_exists = (rc == 0) && S_ISREG(statbuf.st_mode);
    file_len    = file_exists ? statbuf.st_size : 0;
    file_inode  = file_exists ? statbuf.st_ino : 0;

    // if the file exists then 
    //   search table for an existing mapping
    //   if match found then 
    //     increment ref count
    //     return the existing mapped addr
    //   endif
    // endif
    if (file_exists) {
        for (i = 0; i < MAX_FILE_MAP_TBL; i++) {
            struct file_map_tbl_s *x = &file_map_tbl[i];
            if (x->ino == file_inode && x->len == len) {
                x->refcnt++;
                addr = x->addr;
                goto done;
            }
        }
    }

    // if the file doesnt exist, or has the wrong length then
    //   if create flag is set
    //     unlink the file
    //     create the file with the proper length
    //   else
    //     return error
    //   endif
    // endif
    if (!file_exists || file_len != len) {
        if (create_if_needed) {
            char zero=0;
            unlink(path);
            fd = open(path, O_CREAT|O_EXCL|O_RDWR, 0666);
            if (fd < 0) {
                ERROR("failed to open/create %s, %s\n", path, strerror(errno));
                goto done;  
            }
            rc = lseek(fd, len-1, SEEK_SET);
            if (rc != len-1) {
                ERROR("lseek %s failed, %s\n", path, strerror(errno));
                close(fd);
                goto done;  
            }
            rc = write(fd, &zero, 1);
            if (rc != 1) {
                ERROR("write %s failed, %s\n", path, strerror(errno));
                close(fd);
                goto done;  
            }
            close(fd);
        } else {
            ERROR("file doesnt exist or has wrong len, file_exists=%d file_len=%d len=%d\n", 
                   file_exists, file_len, len);
            goto done;  
        }
    }

    // map the file
    fd = open(path, O_CREAT|O_RDWR|O_TRUNC, 0666);
    if (fd < 0) {
        ERROR("failed to open %s, %s\n", path, strerror(errno));
        goto done;  
    }
    addr = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == NULL) {
        ERROR("mmap %s failed, %s\n", path, strerror(errno));
        close(fd);
        goto done;  
    }
    rc = fstat(fd, &statbuf);
    if (rc != 0) {
        ERROR("failed to stat %s, %s\n", path, strerror(errno));
        munmap(addr, len);
        close(fd);
        addr = NULL;
        goto done;  
    }
    close(fd);

    // add mapping entry to table
    file_map_tbl[tbl_idx].addr   = addr;
    file_map_tbl[tbl_idx].len    = len;
    file_map_tbl[tbl_idx].ino    = statbuf.st_ino;
    file_map_tbl[tbl_idx].refcnt = 1;

    // unlock mutex
    pthread_mutex_unlock(&file_map_mutex);

done:
    // return mapped addr
    return addr;
}

void util_unmap_file(void *addr)
{
    int tbl_idx;

    // lock mutex
    pthread_mutex_lock(&file_map_mutex);

    // find addr in file_map_tbl, if not found return error
    for (tbl_idx = 0; tbl_idx < MAX_FILE_MAP_TBL; tbl_idx++) {
        if (file_map_tbl[tbl_idx].addr == addr) {
            break;
        }
    }
    if (tbl_idx == MAX_FILE_MAP_TBL) {
        ERROR("%p not found in file_map_tbl\n", addr);
        goto done;
    }

    // if file_map_tbl entry refcnt is already 0 then return error
    struct file_map_tbl_s *x = &file_map_tbl[tbl_idx];
    if (x->refcnt <= 0) {
        ERROR("refcnt is already zero\n");
        goto done;
    }

    // if file_map_tbl refcnt is 1 then unmap and clear the file_map_tbl entry
    if (x->refcnt == 1) {
        munmap(x->addr, x->len);
        memset(x, 0, sizeof(struct file_map_tbl_s));
    }

done:
    // unlock mutex
    pthread_mutex_unlock(&file_map_mutex);
}

void util_sync_file(void *addr, int len)
{
    unsigned long first_page = (unsigned long)addr & ~(PAGE_SIZE-1);
    unsigned long last_page = ((unsigned long)addr + len) & ~(PAGE_SIZE-1);

    msync((void*)first_page,
          last_page - first_page + PAGE_SIZE,
          MS_SYNC);
}

// -----------------  GET / SET PARAMS  ----------------------

char *util_get_str_param(char *dir, char *name, char *default_value);
void util_set_str_param(char *dir, char *name, char *value);
int util_get_int_param(char *dir, char *name, int default_value);
void util_set_int_param(char *dir, char *name, int value);
void util_print_params(char *dir);

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

static void read_params_file(char *dir)
{
    char s[200], name[100], params_path[100];
    int cnt, n;
    FILE *fp;

    if (strcmp(dir, params_dir) == 0) {
        return;
    }
    strcpy(params_dir, dir);

    INFO("reading params file in dir '%s'\n", dir);

    memset(params, 0, sizeof(params));
    max_params = 0;

    sprintf(params_path, "%s/params", dir);
    fp = fopen(params_path, "r");
    if (fp == NULL) {
        INFO("params file does not exist\n");
        return;
    }

    while (fgets(s, sizeof(s), fp) != NULL) {
        remove_trailing_newline(s);
        n = 0;
        cnt = sscanf(s, "%s = %n", name, &n);
        if (cnt != 1 || n == 0) {
            ERROR("read_params_file '%s'\n", s);
            fclose(fp);
            return;
        }

        params[max_params].name = strdup(name);
        params[max_params].value = strdup(s+n);
        max_params++;
    }

    fclose(fp);

    INFO("max_params=%d\n", max_params);
    for (int i = 0; i < max_params; i++) {
        INFO("  %s = %s\n", params[i].name, params[i].value);
    }
}

static void write_params_file(char *dir)
{
    FILE *fp;
    char params_path[100];

    if (strcmp(dir, params_dir) != 0) {
        ERROR("write_params_file, dir=%s params_dir=%s\n", dir, params_dir);
        return;
    }

    INFO("writing params file in dir '%s'\n", dir);
    INFO("max_params=%d\n", max_params);
    for (int i = 0; i < max_params; i++) {
        INFO("  %s = %s\n", params[i].name, params[i].value);
    }

    sprintf(params_path, "%s/params", dir);
    fp = fopen(params_path, "w");
    if (fp == NULL) {
        ERROR("write_params_file, fopen failed, %s\n", strerror(errno));
        return;
    }

    for (int i = 0; i < max_params; i++) {
        fprintf(fp, "%-16s = %s\n", params[i].name, params[i].value);
    }

    fclose(fp);
}

char *util_get_str_param(char *dir, char *name, char *default_value)
{
    int i;

    // xxx locking needed

    // if haven't read the params file then do so
    read_params_file(dir);

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
            ERROR("params tbl is full\n");
            return default_value;
        }
        params[max_params].name = strdup(name);
        params[max_params].value = strdup(default_value);
        max_params++;
        write_params_file(dir);
        return default_value;
    }
}

void util_set_str_param(char *dir, char *name, char *value)
{
    int i;

    // xxx locking needed

    // if haven't read the params file then do so
    read_params_file(dir);

    // search for matching name
    for (i = 0; i < max_params; i++) {
        if (strcmp(name, params[i].name) == 0) {
            break;
        }
    }

    // if found then
    //   if no change then return
    //   replace its value
    // else
    //   add param to the end
    // endif
    if (i < max_params) {
        if (strcmp(params[i].value, value) == 0) {
            return;
        }
        free(params[i].value);
        params[i].value = strdup(value);
    } else {
        if (max_params >= MAX_PARAMS) {
            ERROR("params tbl is full\n");
            return;
        }
        params[max_params].name = strdup(name);
        params[max_params].value = strdup(value);
        max_params++;
    }

    // write the params file
    write_params_file(dir);
}

int util_get_int_param(char *dir, char *name, int dflt_val)
{
    char  dflt_val_str[20];
    char *value_str;
    int   value_int;
    int   cnt;

    // create the default value string, and
    // call util_get_str_param to get the value_str
    sprintf(dflt_val_str, "%d", dflt_val);
    value_str = util_get_str_param(dir, name, dflt_val_str);

    // convert value_str, returned by util_get_str_param, to value_int
    cnt = sscanf(value_str, "%d", &value_int);

    // the conversion can fail if value_str is not an integer,
    // if the conversion fails then call util_set_int_param, and 
    // return the default value
    if (cnt != 1) {
        util_set_int_param(dir, name, dflt_val);
        return dflt_val;
    }

    // return the integer param value
    return value_int;
}

void util_set_int_param(char *dir, char *name, int value)
{
    char value_str[20];

    // create value string, and
    // call util_set_str_param to set it
    sprintf(value_str, "%d", value);
    util_set_str_param(dir, name, value_str);
}    

void util_print_params(char *dir)
{
    int i;

    // xxx locking

    read_params_file(dir);

    INFO("max_params=%d\n", max_params);
    for (i = 0; i < max_params; i++) {
        INFO("  %s = %s\n", params[i].name, params[i].value);
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
        ERROR("getifaddrs, %s\n", strerror(errno));
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
        ERROR("popen failed, %s\n", strerror(errno));
    }

    return ipaddr;
#endif
}

