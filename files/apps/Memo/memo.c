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

#define EVID_NEW      1
#define EVID_STOP     2
#define EVID_PLAY 100
#define EVID_APPEND   300
#define EVID_DELETE   400

#define MAX_FILENAME 100

//
// variables
//

char *progname;
char *data_dir;

char *filename[MAX_FILENAME];
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
    bool         end_program = false;
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
    while (!end_program) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // get list of audio files
        get_list_of_files();

        // xxx 
        // - display file duration
        // - display volume bar
        // - vertical scrolling
        // - playback duration bar

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

            static char display_name[40];
            strncpy(display_name, filename[idx]+4, 8);  // xxx sanity check filename length

            int y = 100 + ROW2Y(2*idx);

            int file_duration_secs = sdlx_audio_file_duration(data_dir, filename[idx]);

            sdlx_print_init_color(color, COLOR_BLACK);
            loc = sdlx_render_printf(0, y, "%s:%d", display_name, file_duration_secs);
            if (color == COLOR_LIGHT_BLUE || color == COLOR_GREEN) {
                sdlx_register_event(loc, EVID_PLAY+idx);
            }

            if (color == COLOR_LIGHT_BLUE) {
                // register append and delete events
                sdlx_print_init_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
                loc = sdlx_render_printf(COL2X(13.5), y, "%s", "+");
                sdlx_register_event(loc, EVID_APPEND+idx);
                loc = sdlx_render_printf(COL2X(18), y, "%s", "X");
                sdlx_register_event(loc, EVID_DELETE+idx);
            } else {
                // register stop event
                sdlx_print_init_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
                loc = sdlx_render_printf(COL2X(13.5), y, "%s", "STOP");
                sdlx_register_event(loc, EVID_STOP);
            }
        }
        sdlx_print_init_color(COLOR_WHITE, COLOR_BLACK);

        // display volume bar
        int y = sdlx_win_height-300;
        int bar_height = 75;
        if (audio_state.state == AUDIO_STATE_PLAY_FILE) {
            sdlx_render_fill_rect(0, y, sdlx_win_width * audio_state.volume / 100, bar_height, COLOR_GREEN);
            sdlx_render_rect(0, y, sdlx_win_width, bar_height, 2, COLOR_WHITE);
        } else if (audio_state.state == AUDIO_STATE_RECORD || audio_state.state == AUDIO_STATE_RECORD_APPEND) {
            sdlx_render_fill_rect(0, y, sdlx_win_width * audio_state.volume / 100, bar_height, COLOR_RED);
            sdlx_render_rect(0, y, sdlx_win_width, bar_height, 2, COLOR_WHITE);
        }

        // register control events
        sdlx_register_control_events("+", NULL, "X", COLOR_WHITE, COLOR_BLACK, EVID_NEW, 0, EVID_QUIT);

        // present the display
        sdlx_display_present();

        // wait for event, with 100ms timeout
        sdlx_get_event(100000, &event);

        // process events
        if (event.event_id == EVID_NEW) {
            time_t t = time(NULL);
            struct tm tm;
            char new_filename[100];

            localtime_r(&t, &tm);
            sprintf(new_filename, "%04d%02d%02d%02d%02d%02d.raw",
                    tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
            //strftime(new_filename, sizeof(new_filename), "%b%d-%T.raw", &tm);
            printf("INFO %s: EVID_NEW recording to '%s'\n", progname, new_filename);
            sdlx_audio_record(data_dir, new_filename, 30, 2, false);
        } else if (event.event_id == EVID_STOP) {
            printf("INFO %s: EVID_STOP\n", progname);
            sdlx_audio_ctl(AUDIO_REQ_STOP);
        } else if (event.event_id >= EVID_PLAY && event.event_id < EVID_PLAY + max_filename) {
            int idx = event.event_id - EVID_PLAY;
            printf("INFO %s: EVID_PLAY %d\n", progname, idx);
            sdlx_audio_play(data_dir, filename[idx]);
        } else if (event.event_id >= EVID_APPEND && event.event_id < EVID_APPEND + max_filename) {
            int idx = event.event_id - EVID_APPEND;
            printf("INFO %s: EVID_APPEND %d\n", progname, idx);
            sdlx_audio_record(data_dir, filename[idx], 30, 2, true);
        } else if (event.event_id >= EVID_DELETE && event.event_id < EVID_DELETE + max_filename) {
            int idx = event.event_id - EVID_DELETE;
            printf("INFO %s: EVID_DELETE %d\n", progname, idx);
            util_delete_file(data_dir, filename[idx]);
        } else if (event.event_id == EVID_QUIT) {
            end_program = true;
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
    }
    max_filename = 0;

    sprintf(cmd, "cd %s; /bin/ls -1 *.raw", data_dir);
    fp = popen(cmd, "r");
    while (fgets(s, sizeof(s), fp)) {
        remove_trailing_newline(s);
        filename[max_filename++] = strdup(s);
    }
    pclose(fp);

    for (i = 0; i < max_filename; i++) {
        printf("INFO %s:   %d '%s'\n", progname, i, filename[i]);
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
