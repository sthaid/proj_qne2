#include <stdio.h>
#include <stdbool.h>   //xxx why needed
#include <unistd.h>   //xxx why needed

#include <sdl.h>
#include <utils.h>

#include "tester.h"

void tester_proc(void)
{
    int rc;

    printf("verified call between 2 source files\n");

#if 0
    printf("calling open\n");
    sdl_audio_open(48000, 1);
    printf("calling play_tone\n");
    sdl_audio_play_tone(1000, 5000);
    //printf("calling wait\n");
    //sdl_audio_wait();
    //printf("calling close0\n");
    //sdl_audio_close();
#endif

#if 0    
    sdl_audio_open(22050, 1, false);
    //sdl_audio_play_file("/home/haid/super_critical.wav");
    sdl_audio_play_file("super_critical.wav");
    return;
#endif

    static short buff[480000];

    sdl_audio_print_devices_info();
    rc = sdl_audio_open(48000, 1, true);
    if (rc == 0) {
        sdl_audio_record(buff, sizeof(buff)/2);
    } else {
        printf("ERROR: record failed\n");
    }
    sleep(15);
    sdl_audio_close();

    rc = sdl_audio_open(48000, 1, false);
    if (rc != 0) {
        printf("ERROR: open for playback failed\n");
    }
    printf("playing ..\n");
    sdl_audio_play(buff, sizeof(buff)/2, sizeof(buff)/2);
    sdl_audio_wait();
    printf("playing done..\n");
    sdl_audio_close();
}
