#ifndef __UTILS_H__
#define __UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

// -----------------  TIME  ----------------------------------

long util_microsec_timer(void);
long util_get_real_time_microsec(void);
char *util_time2str(char * str, long us, int gmt, int display_ms, int display_date);

// -----------------  FILE READ/WRITE/DELETE  ----------------

int util_write_file(char *dir, char *fn, void *data, int len);
void *util_read_file(char *dir, char *fn, int *len);
void util_delete_file(char *dir, char *fn);

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

// -----------------  JSON  ----------------------------------

#define JSON_TYPE_UNDEFINED 0
#define JSON_TYPE_FLAG      1
#define JSON_TYPE_NUMBER    2
#define JSON_TYPE_STRING    3
#define JSON_TYPE_ARRAY     4
#define JSON_TYPE_OBJECT    5

typedef struct {
    int type;
    union {
        bool   flag;
        double number;
        char  *string;
        void  *array;
        void  *object;
    } u;
} json_value_t;

void *util_json_parse(char *str);
void util_json_free(void *json_root);
json_value_t *util_json_get_value(void *json_item, ...);

// -----------------  JSON  ----------------------------------

void util_get_location(double *latitude, double *longitude, double *altitude);

#ifdef __cplusplus
}
#endif

#endif
