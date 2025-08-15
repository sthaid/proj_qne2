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

void log_init(char *logfile);
void log_msg(char * lvl, const char * func, char * fmt, ...) __attribute__ ((format (printf, 3, 4)));
void log_clear(void);
int log_size(void);

