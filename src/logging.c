#include <std_hdrs.h>

#include <logging.h>

#ifdef ANDROID
    //#define USE_ANDROID_LOGGING
    #ifdef USE_ANDROID_LOGGING
        #include <SDL.h>
    #endif
#endif

#define MAX_TIME_STR 100

static long get_real_time_us(void);
static char * time2str(char * str, long us, bool gmt, bool display_ms, bool display_date);

// ----------------- LOGGING -----------------

void init_logging(char *logfile)
{
    FILE *fp;
    int rc;

    if (logfile) {
        fp = freopen(logfile, "a", stdout);
        if (fp == NULL) {
            ERROR("failed to reopen stdout to file '%s', %s\n", logfile, strerror(errno));
            return;
        }
        rc = dup2(fileno(stdout), fileno(stderr));
        if (rc != 0) {
            ERROR("failed to dup stdout to stderr, %s\n", strerror(errno));
            return;
        }
    }

    setlinebuf(stdout);
    setlinebuf(stderr);

    fprintf(stdout, "test print to stdout\n");  //xxx temp
    fprintf(stderr, "test print to stderr\n");
}

void logmsg(char *lvl, const char *func, char *fmt, ...)
{
    va_list ap;
    char    msg[1000];
    char    time_str[MAX_TIME_STR];
    int     len;

    // construct msg
    va_start(ap, fmt);
    len = vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    // remove terminating newline
    if (len > 0 && msg[len-1] == '\n') {
        msg[len-1] = '\0';
        len--;
    }

#ifdef USE_ANDROID_LOGGING
    // log the message, to the Android log;
    // use 'adb -s SDL/APP' to monitor the Android log
    if (strcmp(lvl, "INFO") == 0) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "%s %s: %s\n",
                    lvl, func, msg);
    } else if (strcmp(lvl, "WARN") == 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "%s %s: %s\n",
                    lvl, func, msg);
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "%s %s: %s\n",
                     lvl, func, msg);
    }
    return;
#endif

    // log using printf to stderr
    time2str(time_str, get_real_time_us(), false, true, true),
    fprintf(stderr, "%s %s %s: %s\n", time_str, lvl, func, msg);
}

// -----------------  LOCAL  -------------------------

static long get_real_time_us(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME,&ts);
    return ((long)ts.tv_sec * 1000000) + ((long)ts.tv_nsec / 1000);
}

static char * time2str(char * str, long us, bool gmt, bool display_ms, bool display_date)
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

