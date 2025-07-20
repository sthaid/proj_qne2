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

void init_logging(char *logfile);
void logmsg(char * lvl, const char * func, char * fmt, ...) __attribute__ ((format (printf, 3, 4)));

// -----------------  TIME  ------------------

#define MAX_TIME_STR 50
unsigned long microsec_timer(void);
unsigned long get_real_time_us(void);
char * time2str(char * str, unsigned long us, bool gmt, bool display_ms, bool display_date);

// -----------------  NETWORKING  ------------

char * sock_addr_to_str(char * s, int slen, struct sockaddr * addr);
bool is_socket_connected(int socket_fd) ;

// ----------------- MISC  -------------------

void get_file_info(char *pathname, size_t *size, time_t *mtime);
void remove_trailing_newline(char *s);

