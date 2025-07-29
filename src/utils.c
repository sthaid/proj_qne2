#include <std_hdrs.h>

#include <utils.h>

// xxx pthread_create_detached

// ----------------- TIME --------------------

long util_microsec_timer(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC,&ts);
    return  ((long)ts.tv_sec * 1000000) + ((long)ts.tv_nsec / 1000);
}

long util_get_real_time_us(void)  // xxx use microsec instead of us?
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

static void remove_trailing_newline(char *s)
{
    int len = strlen(s);

    if (len > 0) {
        s[len-1] = '\0';
    }
}

static void read_params_file(void)
{
    static bool first_call = true;
    char s[200], name[100];
    int cnt, n;
    FILE *fp;

    if (first_call == false) {
        return;
    }
    first_call = false;
    
    fp = fopen("params", "r");
    if (fp == NULL) {
        return;
    }

    while (fgets(s, sizeof(s), fp) != NULL) {
        remove_trailing_newline(s);
        n = 0;
        cnt = sscanf(s, "%s = %n", name, &n);
        if (cnt != 1 || n == 0) {
            printf("ERROR: read_params_file '%s'\n", s);
            fclose(fp);
            return;
        }

        params[max_params].name = strdup(name);
        params[max_params].value = strdup(s+n);
        max_params++;
    }

    fclose(fp);

    util_print_params();
}

static void write_params_file(void)
{
    FILE *fp;
    int i;

    fp = fopen("params", "w");
    if (fp == NULL) {
        printf("ERROR: write_param_file, fopen failed, %s\n", strerror(errno));
        return;
    }

    for (i = 0; i < max_params; i++) {
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
            printf("ERROR: params tbl is full\n");
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
            printf("ERROR: params tbl is full\n");
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

    printf("max_params=%d\n", max_params);
    for (i = 0; i < max_params; i++) {
        printf("  %s = %s\n", params[i].name, params[i].value);
    }
}

