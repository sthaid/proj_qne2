#ifndef __LOC_DATA_H__
#define __LOC_DATA_H__

// private header file, do not include in client app

// xxx maybe rename this file OR some other solution fro these 2 vars
char         *progname;
char         *data_dir;

#define MAX_NAME 32  // xxx fix

typedef struct {
    double latitude;
    double longitude;
    char   name[MAX_NAME];
} loc_data_t;

loc_data_t   *loc_data;
int           max_loc_data;

int init_loc_data(void);
void find_closest_loc_data(double latitude, double longitude, char *name, double *miles);
int download_country_loc_data(char *id);

// xxx add free routine for cleanup

#endif
