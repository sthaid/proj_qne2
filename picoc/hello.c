//xxx temp file
#include <stdio.h>
#include <stdbool.h>
//#include <sdl.h>



int x[] = {1,2,3};


int main(int argc, char **argv)
{
    int i;

int za[] = { 15<<16 | 8,  15<<16 | 9, 777 };


    printf("STARTING\n");

    static int inten[2] = {1*5,2*5};
    printf("inten=%d %d  sizeof=%zd\n", inten[0], inten[1], sizeof(inten));


#ifdef PICOC_VERSION
    printf("hello world %s\n", PICOC_VERSION);
#else
    printf("hello world\n");
#endif

    printf("argc %d\n", argc);
    for (i = 0; i < argc; i++) {
        printf("argv[%d] = '%s'\n", i, argv[i]);
    }

    //printf("EVID_SWIPE_DOWN = %d\n", EVID_SWIPE_DOWN);
    //printf("COLOR_RED = 0x%x\n", COLOR_RED);

    printf("%d %d %d - %zd\n", x[0], x[1], x[2], sizeof(x));

    printf("%zd  %x  %x  %x \n", sizeof(za), za[0], za[1], za[2]);

    return 0;
}
