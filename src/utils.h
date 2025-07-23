// -----------------  TIME  ------------------

long util_microsec_timer(void);
long util_get_real_time_us(void);
char *util_time2str(char * str, long us, int gmt, int display_ms, int display_date);

// -----------------  FILE READ/WRITE  -------

int util_write_file(char *path, void *data, int len);
void *util_read_file(char *path, int *len);

