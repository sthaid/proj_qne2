#include <stdio.h>
#include <stdbool.h>   //xxx why needed

#include <sdl.h>
#include <utils.h>

#include "tester.h"

void tester_proc(void)
{
    printf("verified call between 2 source files\n");

    printf("calling open\n");
    sdl_audio_open(48000, 1);
    printf("calling play_tone\n");
    sdl_audio_play_tone(1000, 5000);
    //printf("calling wait\n");
    //sdl_audio_wait();
    //printf("calling close0\n");
    //sdl_audio_close();
}
