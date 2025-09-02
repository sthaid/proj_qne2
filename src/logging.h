#ifndef __LOGGING_H__
#define __LOGGING_H__

#define INFO(fmt, args...) \
    do { \
        log_msg("INFO", __func__, fmt, ## args); \
    } while (0)
#define WARN(fmt, args...) \
    do { \
        log_msg("WARN", __func__, fmt, ## args); \
    } while (0)
#define ERROR(fmt, args...) \
    do { \
        log_msg("ERROR", __func__, fmt, ## args); \
    } while (0)

int log_init(void);
void log_msg(char * lvl, const char * func, char * fmt, ...) __attribute__ ((format (printf, 3, 4)));

#endif
