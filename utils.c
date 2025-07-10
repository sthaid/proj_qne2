#include <std_hdrs.h>

#include <utils.h>

//#define USE_ANDROID_LOG
#ifdef USE_ANDROID_LOG
#include <SDL.h>
#endif

// ----------------- LOGGING -----------------

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

#ifndef USE_ANDROID_LOG
    // print the message, stdout has been redirected to the log file, in main.c;
    // use './qne logcat' to monitor the log xxx
    time2str(time_str, get_real_time_us(), false, true, true),
    printf("%s %s %s: %s\n", time_str, lvl, func, msg);
#else
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
    } else if (strcmp(lvl, "FATAL") == 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "%s %s: %s\n",
                        lvl, func, msg);
    } else if (strcmp(lvl, "DEBUG") == 0) {
        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
                     "%s %s: %s\n",
                     lvl, func, msg);
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "%s %s: %s\n",
                     lvl, func, msg);
    }
#endif
}

// ----------------- TIME --------------------

unsigned long microsec_timer(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC,&ts);
    return  ((unsigned long)ts.tv_sec * 1000000) + ((unsigned long)ts.tv_nsec / 1000);
}

unsigned long get_real_time_us(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME,&ts);
    return ((unsigned long)ts.tv_sec * 1000000) + ((unsigned long)ts.tv_nsec / 1000);
}

char * time2str(char * str, unsigned long us, bool gmt, bool display_ms, bool display_date)
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

// ----------------- NETWORKING --------------

char * sock_addr_to_str(char * s, int slen, struct sockaddr * addr)
{
    char addr_str[100];
    int port2;

    if (addr->sa_family == AF_INET) {
        inet_ntop(AF_INET,
                  &((struct sockaddr_in*)addr)->sin_addr,
                  addr_str, sizeof(addr_str));
        port2 = ((struct sockaddr_in*)addr)->sin_port;
    } else if (addr->sa_family == AF_INET6) {
        inet_ntop(AF_INET6,
                  &((struct sockaddr_in6*)addr)->sin6_addr,
                 addr_str, sizeof(addr_str));
        port2 = ((struct sockaddr_in6*)addr)->sin6_port;
    } else {
        snprintf(s,slen,"Invalid AddrFamily %d", addr->sa_family);
        return s;
    }

    snprintf(s,slen,"%s:%d",addr_str,ntohs(port2));
    return s;
}

bool is_socket_connected(int socket_fd)
{
    int error = 0;
    int ret;
    socklen_t len = sizeof(error);

    ret = getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &error, &len);

    return ret == 0 && error == 0;
}

// ----------------- MISC --------------------

void get_file_info(char *pathname, size_t *size, time_t *mtime)
{
    struct stat statbuf;

    if (lstat(pathname, &statbuf) != 0) {
        if (size) *size = 0;
        if (mtime) *mtime = 0;
        return;
    }

    if (size) *size = statbuf.st_size;
    if (mtime) *mtime = statbuf.st_mtime;
}

void remove_trailing_newline(char *s)
{
    int len = strlen(s);

    if (len > 0) {
        s[len-1] = '\0';
    }
}

// xxx pthread_create_detached
