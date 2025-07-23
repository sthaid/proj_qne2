#include <stdio.h>
#include <stdbool.h>

#include <sdl.h>
#include <utils.h>

#define MAX 1000000   // 1 million

bool composite[MAX];

int main(int argc, char **argv)
{
    int n, k;
    long start, duration;

    start = util_microsec_timer();
    for (n = 2; n < MAX; n++) {
        if (!composite[n]) {
            for (k = 2*n; k < MAX; k+=n) {
                composite[k] = true;
            }
        }
    }
    duration = util_microsec_timer() - start;
    printf("duration = %f secs\n", duration/1000000.);

    for (n = 2; n < 20; n++) {
        if (!composite[n]) {
            printf("%d\n", n);
        }
    }

    return 0;
}

