#include <stdio.h>

#include <sdl.h>
#include <utils.h>

#include "tester.h"

void tester_proc(void)
{
    printf("verified call between 2 source files\n");

    sdl_audio();
}
