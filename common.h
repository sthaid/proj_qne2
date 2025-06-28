#if 0
#include <libgen.h>
#include <limits.h>
#include <math.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <SDL.h>
#endif
// xxx use int vs uint64_t

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

// -------------------------------------------

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

void logmsg(char * lvl, const char * func, char * fmt, ...) __attribute__ ((format (printf, 3, 4)));

// -------------------------------------------

#define MAX_TIME_STR 50

uint64_t microsec_timer(void);
uint64_t get_real_time_us(void);
char * time2str(char * str, int64_t us, bool gmt, bool display_ms, bool display_date);

// -------------------------------------------

#define MAX_FONT_PTSIZE       200

int32_t sdl_init(bool swap_white_black);
void sdl_exit(void);
