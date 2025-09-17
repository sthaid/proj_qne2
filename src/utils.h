#ifndef __UTILS_H__
#define __UTILS_H__

// -----------------  TIME  ----------------------------------

long util_microsec_timer(void);
long util_get_real_time_microsec(void);
char *util_time2str(char * str, long us, int gmt, int display_ms, int display_date);

// -----------------  FILE READ/WRITE  -----------------------

int util_write_file(char *dir, char *fn, void *data, int len);
void *util_read_file(char *dir, char *fn, int *len);

// -----------------  FILE MAP -------------------------------

void *util_map_file(char *dir, char *file, int len, bool create_if_needed);
void util_unmap_file(void *addr);
void util_sync_file(void *addr, int len);

// -----------------  GET / SET PARAMS  ----------------------

char *util_get_str_param(char *dir, char *name, char *default_value);
void util_set_str_param(char *dir, char *name, char *value);
int util_get_int_param(char *dir, char *name, int default_value);
void util_set_int_param(char *dir, char *name, int value);
void util_print_params(char *dir);

// -----------------  NETWORK  -------------------------------

char *util_get_ipaddr(void);

#endif
