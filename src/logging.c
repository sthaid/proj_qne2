#include <std_hdrs.h>
#include <logging.h>

#ifdef ANDROID
    #include <SDL3/SDL.h>
    #define ANDROID_LOG_FIFO "log_fifo"
    static void *android_logging_thread(void *cx);
#endif

#ifdef ANDROID

// ----------------- ANDROID LOGGING -----------------

// xxx comments needed
int log_init(void)
{
    int rc;
    FILE *fp;
    pthread_t tid;

    setlinebuf(stdout);
    setlinebuf(stderr);

    mkfifo(ANDROID_LOG_FIFO, 0666);

    pthread_create(&tid, NULL, android_logging_thread, NULL);

    fp = freopen(ANDROID_LOG_FIFO, "w", stdout);
    if (fp == NULL) {
        ERROR("failed to reopen stdout, %s\n", strerror(errno));
        return -1;
    }
    setlinebuf(stdout);

    rc = dup2(fileno(stdout), fileno(stderr));
    if (rc < 0) {
        ERROR("failed to dup stdout to stderr, %s\n", strerror(errno));
        return -1;
    }

    fprintf(stdout, "test print to stdout\n");  // xxx temp
    fprintf(stderr, "test print to stderr\n");  // xxx temp

    return 0;
}

void log_msg(char *lvl, const char *func, char *fmt, ...)
{
    va_list ap;
    char    msg[1000];
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

    // log to stderr, which is redirected to the log fifo
    fprintf(stderr, "%s %s: %s\n", lvl, func, msg);
}

// ----------------- ANDROID LOGGING THREAD   -----------------

static void *android_logging_thread(void *cx)
{
    char buff[10000];
    int len;
    char *buffp, *p;

    int fd = open(ANDROID_LOG_FIFO, O_RDONLY);
    if (fd < 0) {
        return NULL;
    }

    while (true) {
        len = read(fd, buff, sizeof(buff)-1);
        if (len <= 0) {
            goto done;
        }

        if (buff[len-1] != '\n') {
            buff[len-1] = '\n';
        }
        buff[len] = '\0';

        buffp = buff;
        while (true) {
            p = strchr(buffp, '\n');
            if (p == NULL) {
                break;
            }

            *p = '\0';
            if (strncmp(buffp, "EZAPP", 5) != 0) {
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "EZAPP %s", buffp);
            }
            buffp = p + 1;
        }
    }

done:
    return NULL;
}
    
#else

// ------------- NOT ANDROID LOGGING SUPPORT ---------

int log_init(void)
{
    setlinebuf(stdout);
    setlinebuf(stderr);
    return 0;
}

void log_msg(char *lvl, const char *func, char *fmt, ...)
{
    va_list ap;
    char    msg[1000];
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

    // log to stderr, which is redirected to the log fifo
    fprintf(stderr, "%s %s: %s\n", lvl, func, msg);
}

#endif
