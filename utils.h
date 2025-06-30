// -----------------  LOGGING  ---------------

#define INFO(fmt, args...) \
    do { \
        logmsg("INFO", __func__, fmt, ## args); \
    } while (0)
#define WARN(fmt, args...) \
    do { \
        logmsg("WARN", __func__, fmt, ## args); \
    } while (0)
#define ERROR(fmt, args...) \
    do { \
        logmsg("ERROR", __func__, fmt, ## args); \
    } while (0)
#define FATAL(fmt, args...) \
    do { \
        logmsg("FATAL", __func__, fmt, ## args); \
        exit(1); \
    } while (0)

void logmsg(char * lvl, const char * func, char * fmt, ...) __attribute__ ((format (printf, 3, 4)));

// -----------------  TIME  ------------------

#define MAX_TIME_STR 50

uint64_t microsec_timer(void);
uint64_t get_real_time_us(void);
char * time2str(char * str, int64_t us, bool gmt, bool display_ms, bool display_date);
