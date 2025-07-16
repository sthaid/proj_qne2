//xxx temp file
#include <stdio.h>
#include <stdbool.h>
#include <sdl.h>

int main(int argc, char **argv)
{
    int i;

#ifdef PICOC_VERSION
    printf("hello world %s\n", PICOC_VERSION);
#else
    printf("hello world\n");
#endif

    printf("argc %d\n", argc);
    for (i = 0; i < argc; i++) {
        printf("argv[%d] = '%s'\n", i, argv[i]);
    }

    printf("EVID_SWIPE_DOWN = %d\n", EVID_SWIPE_DOWN);
    printf("COLOR_RED = 0x%x\n", COLOR_RED);


    return 0;
}
