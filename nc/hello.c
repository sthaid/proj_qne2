#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

int main(int argc, char **argv)
{
    time_t t;

    printf("sizoef(time_t)    = %zd\n", sizeof(time_t));
    printf("sizoef(char)      = %zd\n", sizeof(char));
    printf("sizoef(short)     = %zd\n", sizeof(short));
    printf("sizoef(int)       = %zd\n", sizeof(int));
    printf("sizoef(long)      = %zd\n", sizeof(long));
    printf("sizoef(size_t)    = %zd\n", sizeof(size_t));
    printf("sizoef(off_t)     = %zd\n", sizeof(off_t));
    printf("sizeof(123UL)     = %zd\n", sizeof(123UL));
    printf("sizeof(123)       = %zd\n", sizeof(123));

    while (true) {
        time(&t);
        printf("hello %d %d %lld\n", t, 123, 1234567890000UL);

        sleep(1);

        time(&t);
        printf("bye %d\n", t);
    }

    return 0;
}
