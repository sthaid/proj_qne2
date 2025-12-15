#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include <sdlx.h>
#include <utils.h>

//
// defines
//

#define EVID_NEW 1
#define EVID_FILENAME 100

#define MAX_FILENAME 100

//
// variables
//

char *progname;
char *data_dir;

char *filename[MAX_FILENAME];
char *display_name[MAX_FILENAME];
int   state[MAX_FILENAME];
int   max_filename;

//
// prototypes
//

void get_list_of_files(void);
void remove_trailing_newline(char *s);

// -----------------  MAIN  ------------------------------------------
    
int main(int argc, char **argv)
{
    int          rc;
    sdlx_event_t event;
    bool         done = false;
    sdlx_audio_state_t audio_state;

    // save args
    if (argc != 2) {
        printf("ERROR: data_dir arg expected\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    // init sdl video and audio subsystems
    rc = sdlx_init(SUBSYS_VIDEO|SUBSYS_AUDIO);
    if (rc != 0) {
        printf("ERROR %s: sdlx_init failed\n", progname);
        return 1;
    }

    // runtime loop
    while (!done) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // get list of audio files
        get_list_of_files();

        // xxx 
        // - green when playing
        // - red when recording
        // - shorten name

        // xxx comment
        sdlx_audio_state(&audio_state);
        memset(&state, 0, sizeof(state));
        if (audio_state.state != AUDIO_STATE_IDLE) {
            for (int idx = 0; idx < max_filename; idx++) {
                if (strstr(audio_state.filename, filename[idx]) != NULL) {
                    state[idx] = audio_state.state;
                    break;
                }
            }
        }

        // display the audio filename, followed by events to append, or delete
        for (int idx = 0; idx < max_filename; idx++) {
            sdlx_loc_t *loc;
            int color;

            color = (state[idx] == AUDIO_STATE_PLAY_FILE     ? COLOR_GREEN : 
                     state[idx] == AUDIO_STATE_RECORD        ? COLOR_RED :
                     state[idx] == AUDIO_STATE_RECORD_APPEND ? COLOR_RED :
                                                               COLOR_LIGHT_BLUE);

            sdlx_print_init_color(color, COLOR_BLACK);
            loc = sdlx_render_printf(0, 100 + ROW2Y(1.5*idx), "%s", display_name[idx]);
            sdlx_register_event(loc, EVID_FILENAME+idx);
        }
        sdlx_print_init_color(COLOR_WHITE, COLOR_BLACK);

        // register control events
        sdlx_register_control_events("+", NULL, "X", COLOR_WHITE, COLOR_BLACK, EVID_NEW, 0, EVID_QUIT);

        // present the display
        sdlx_display_present();

        // wait for event, with infinite timeout
        sdlx_get_event(-1, &event);

        // process events
        switch (event.event_id) {
        case EVID_NEW: {
            time_t t = time(NULL);
            struct tm tm;
            char new_filename[100];

            localtime_r(&t, &tm);
            //sprintf(new_filename, "%04d-%02d-%02dT%02d:%02d",
            //        tm_year+1900, tm_mon+1, tm_mday, tm_hour, tm_min);
            strftime(new_filename, sizeof(new_filename), "%b%d-%T.raw", &tm);
            printf("INFO %s: EVID_NEW recording to '%s'\n", progname, new_filename);
            sdlx_audio_record(data_dir, new_filename, 30, 2, false);
            break; }
        case EVID_FILENAME: {
            int idx = event.event_id - EVID_FILENAME;
            printf("INFO %s: EVID_FILENAME %d\n", progname, idx);
            sdlx_audio_play(data_dir, filename[idx]);
            break; }
        case EVID_QUIT:
            done = true;
            break;
        }
    }

    // cleanup and end program
    sdlx_quit(SUBSYS_VIDEO|SUBSYS_AUDIO);
    printf("INFO %s: terminating\n", progname);
    return 0;
}

void get_list_of_files(void)
{
    FILE *fp;
    int   i;
    char  cmd[100], s[100];
    long  mtime;

    static long mtime_last;

    // return if no change
    mtime = util_file_mtime(data_dir, NULL);
    if (mtime == mtime_last) {
        return;
    }
    mtime_last = mtime;

    printf("INFO %s: updating filenames\n", progname);

    for (i = 0; i < max_filename; i++) {
        free(filename[i]); // xxx free at term
        filename[i] = NULL;
        free(display_name[i]);
        display_name[i] = NULL;
    }
    max_filename = 0;

    sprintf(cmd, "cd %s; /bin/ls -1tr *.raw", data_dir);
    fp = popen(cmd, "r");
    while (fgets(s, sizeof(s), fp)) {
        remove_trailing_newline(s);
        filename[max_filename++] = strdup(s);
    }
    pclose(fp);

    static char dispname[12];
    for (i = 0; i < max_filename; i++) {
        printf("i = %d\n", i);
        strncpy(dispname, filename[i], 11);
        printf("dispname = '%s'\n", dispname);
        display_name[i] = strdup(dispname);
    }

    for (i = 0; i < max_filename; i++) {
        printf("INFO %s: %d '%s' '%s'\n", progname, i, filename[i], display_name[i]);
    }
}

void remove_trailing_newline(char *s)
{
    int len = strlen(s);   

    if (len > 0 && s[len-1] == '\n') {
        s[len-1] = '\0';   
    }
}

#if 0
xxxxxxxxxxxxxxxxxxxx
Dec08-08:45  App Del

12-08 08:45
Dec08_08:45
Dec08-08:45
YYYY-MM-DDTHH:MM:SS
YYYY-MM-DDTHH:MM:SS
2025-12-14T14:30:15

    memset(&tm_gmt,0,sizeof(tm_gmt));
    tm_gmt.tm_sec   = seconds;
    tm_gmt.tm_min   = minute;
    tm_gmt.tm_hour  = hour;
    tm_gmt.tm_mday  = day;
    tm_gmt.tm_mon   = month - 1;     // 0 to 11
    tm_gmt.tm_year  = year - 1900;   // based 1900
    t = timegm(&tm_gmt);
#endif
