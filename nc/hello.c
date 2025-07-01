#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

typedef short int16_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
} rect_t;

rect_t proc(void)
{
    rect_t r;
    r.x = 1;
    r.y = 2;
    r.w = 3;
    r.h = 4;
    return r;
}

int main(int argc, char **argv)
{
    time_t t;
    rect_t rect;

    printf("sizoef(time_t)    = %zd\n", sizeof(time_t));
    printf("sizoef(char)      = %zd\n", sizeof(char));
    printf("sizoef(short)     = %zd\n", sizeof(short));
    printf("sizoef(int)       = %zd\n", sizeof(int));
    printf("sizoef(long)      = %zd\n", sizeof(long));
    printf("sizoef(size_t)    = %zd\n", sizeof(size_t));
    printf("sizoef(off_t)     = %zd\n", sizeof(off_t));
    printf("sizeof(123UL)     = %zd\n", sizeof(123UL));
    printf("sizeof(123)       = %zd\n", sizeof(123));

    rect = proc();
    printf("%d %d %d %d\n",
       rect.x, rect.y, rect.w, rect.h);
    return 0;

    while (true) {
        time(&t);
        printf("hello %d %d %lld\n", t, 123, 1234567890000UL);

        sleep(1);

        time(&t);
        printf("bye %d\n", t);
    }

    return 0;
}
