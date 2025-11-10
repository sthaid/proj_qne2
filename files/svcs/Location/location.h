#ifndef __LOCATION_H__
#define __LOCATION_H__

#define SVC_LOCATION_REQ_GET_LOC_INFO       10
#define SVC_LOCATION_REQ_ADD_COUNTRY_INFO   11
#define SVC_LOCATION_REQ_DEL_COUNTRY_INFO   12
#define SVC_LOCATION_REQ_LIST_COUNTRY_INFO  13

#define LOC_HIST_FILE_MAGIC 0x55aa5501
#define LOC_HIST_FILENAME   "loc_hist"
#define MAX_LOC_HIST        1000

typedef struct {
    int magic;
    int count;
    struct loc_hist_entry_s {
        unsigned long t;
        double        latitude;
        double        longitude;
        char          name[MAX_NAME];
        double        miles;
    } loc[MAX_LOC_HIST];
} loc_hist_t;

#endif
