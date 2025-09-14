#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include <sdl.h>

#define SECS_PER_HOUR 10  // xxx 3600
static char *progname;
static char *data_dir;
static int   id;

// xxx temp
void *sdl_plot_open(char *dir, char *file);
void sdl_plot_close(void *cx);
void sdl_plot_add_value(void *cx, double value, time_t t);

void sdl_plot_test(void *cx);

int main(int argc, char **argv)
{
    int    rc;
    time_t t;
    int    hour, hour_last=0;
    bool   hour_changed;
    void  *pressure_file;
    double pressure_value;

    // save args
    progname = argv[0];
    data_dir = argv[1];
    sscanf(argv[2], "%d", &id);

    // print starting message
    printf("INFO %s: starting, data_dir = %s id = %d \n", progname, data_dir, id);

    // init the SDL sensor subsystem
    rc = sdl_init(SUBSYS_SENSOR);
    if (rc != 0) {
        printf("ERROR %s: failed to init SUBSYS_SENSOR\n", progname);
        return 1;
    }

    // xxx only create files for sensors that exist

    // open sensor data files
    pressure_file = sdl_plot_open(data_dir, "pressure.dat");
    if (pressure_file == NULL) {
        printf("ERROR %s: sdl_plot_open %s %s failed\n",
               progname, data_dir, "pressure.dat");
        return 1;
    }

    // xxx
    //sdl_plot_test(pressure_file);
    //return 1;

    // loop until stop request recieved
    while (true) {
        // if request to stop received then break out of loop
        if (stop_requested[id]) {
            printf("INFO %s: got stop request\n", progname);
            break;
        }

        // determine if the hour has changed
        t = time(NULL);
        hour = t / SECS_PER_HOUR;
        hour_changed = (hour != hour_last);
        hour_last = hour;

        // read sensor value and save value to data file
        if (hour_changed) {
            rc = sdl_sensor_read_pressure(&pressure_value);
            //xxx if (rc == 0) {
                pressure_value = 1234;
                sdl_plot_add_value(pressure_file, pressure_value, t);
            //xxx }
        }

        // sleep 10 secs
        sleep(1); //xxx 10
    }

    // cleanup
    sdl_plot_close(pressure_file);
    sdl_exit();

    // end program
    printf("INFO %s: terminating\n", progname);
    return 0;
}

// ===================================================================
// ===================================================================
// ===================================================================

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <math.h>

#define MB 0x100000

#define DATA_FILE_MAGIC 0x77777777

#define DATA_FILE_SIZE MB

#define member_size(type, member) (sizeof( ((type *)0)->member )) //xxx

#define MAX_DATA ((DATA_FILE_SIZE - 128) / sizeof(double)) // xxx 128

typedef struct {
    // header
    struct {
        int magic;
        int start_time;  // rounded down to hour boundary
        int reserved[30];
    } hdr;
    // data
    double data[MAX_DATA];
} data_file_t;


static unsigned long pagesize;

void *sdl_plot_open(char *dir, char *file)
{
    unsigned char zero=0;
    bool created;
    int fd, rc, i;
    data_file_t *x;
    char path[100];
    struct stat statbuf;

    // xxx move this
    if (pagesize == 0) {
        pagesize = getpagesize();
    }

    // xxx debug prints
    printf("INFO %s: pagesize = %ld\n", progname, pagesize);
    printf("INFO %s: MAX_DATA = %ld\n", progname, MAX_DATA);

    // open file, create if needed
    sprintf(path, "%s/%s", dir, file);
    fd = open(path, O_CREAT|O_RDWR, 0666);
    if (fd < 0) {
        printf("ERROR %s: failed to open %s, %s\n", progname, path, strerror(errno));
        return NULL;
    }

    // determine if file was just created
    fstat(fd, &statbuf);
    created = (statbuf.st_size == 0);
    printf("INFO %s: created = %d\n", progname, created);

    // if just created then initialize the file
    if (created) {
        rc = lseek(fd, DATA_FILE_SIZE-1, SEEK_SET);
        if (rc != DATA_FILE_SIZE-1) {
            printf("ERROR %s: lseek failed, %s\n", progname, strerror(errno));
            return NULL;
        }
        rc = write(fd, &zero, 1);
        if (rc != 1) {
            printf("ERROR %s: write failed, %s\n", progname, strerror(errno));
            return NULL;
        }
    }

    // xxx temp print file size
    fstat(fd, &statbuf);
    printf("INFO %s: file size = %ld\n", progname, statbuf.st_size);

    // memory map the file
    x = mmap(NULL, DATA_FILE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (x == NULL) {
        printf("ERROR %s: mmap failed, %s\n", progname, strerror(errno));
        return NULL;
    }

    // init the memory mapped file
    if (created) {
        x->hdr.magic = DATA_FILE_MAGIC;
        x->hdr.start_time = (time(NULL) / SECS_PER_HOUR) * SECS_PER_HOUR;
        for (i = 0; i < MAX_DATA; i++) {
            x->data[i] = NAN;
        }
        rc = msync(x, DATA_FILE_SIZE, MS_SYNC);
        if (rc != 0) {
            printf("ERROR %s: msync failed, %s\n", progname, strerror(errno));
            return NULL;
        }
    }

    // close fd
    close(fd);

    // return the context, which is used in calls to other sdl_plot routines
    printf("INFO %s: sdl_plot_open success\n", progname);
    return x;
}

void sdl_plot_close(void *cx)
{
    int rc;

    rc = munmap(cx, DATA_FILE_SIZE);
    if (rc != 0) {
        printf("ERROR %s: munmap failed, %s\n", progname, strerror(errno));
    }
}

void sdl_plot_add_value(void *cx, double value, time_t t)
{
    data_file_t *x = (data_file_t*)cx;
    unsigned long addr;
    int idx, rc;

    idx = (t - x->hdr.start_time) / SECS_PER_HOUR;
    printf("INFO %s: idx = %d\n", progname, idx);

    if (idx < 0 || idx >= MAX_DATA) {
        printf("ERROR %s: idx out of range\n", progname);
        return;
    }
    
    x->data[idx] = value;

    addr = (unsigned long)(&x->data[idx]);
    addr = (unsigned long)&pagesize & ~(pagesize-1);
    rc = msync((void*)addr, pagesize, MS_ASYNC);
    if (rc != 0) {
        printf("ERROR %s: msync failed, %s\n", progname, strerror(errno));
    }
}

// xxx test code
void sdl_plot_test(void *cx)
{
    int i;
    data_file_t *x = (data_file_t*)cx;

    for (i = 0; i < MAX_DATA; i++) {
        if (!isnan(x->data[i])) {
            printf("%d %f\n", i, x->data[i]);
        }
    }
}
