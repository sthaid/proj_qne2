//xxx temp file
#include <stdio.h>

int main(int argc, char **argv)
{
    int i;

    printf("argc %d\n", argc);
    for (i = 0; i < argc; i++) {
        printf("argv[%d] = '%s'\n", i, argv[i]);
    }

#ifdef PICOC_VERSION
    printf("hello world %s\n", PICOC_VERSION);
#else
    printf("hello world\n");
#endif

    return 0;
}
