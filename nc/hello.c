#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include <picoc_unix.h>

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

#define TEST_VAL 567801

int main(int argc, char **argv)
{
    time_t t;
    rect_t rect;
    struct sdl_rect *loc;

    printf("sizoef(char)      = %zd\n", sizeof(char));
    printf("sizoef(short)     = %zd\n", sizeof(short));
    printf("sizoef(int)       = %zd\n", sizeof(int));
    printf("sizoef(long)      = %zd\n", sizeof(long));
    printf("sizoef(size_t)    = %zd\n", sizeof(size_t));
    printf("sizoef(off_t)     = %zd\n", sizeof(off_t));
    printf("sizoef(time_t)    = %zd\n", sizeof(time_t));
    printf("sizeof(123)       = %zd\n", sizeof(123));
    printf("sizeof(123UL)     = %zd\n", sizeof(123UL));

    printf("sizeof sdl_rect = %zd\n", sizeof(struct sdl_rect));

    printf("calling test2\n");
    loc = test2();
    printf("%d %d %d %d\n", 
       loc->x, loc->y, loc->w, loc->h);

    printf("calling test\n");
    test(TEST_VAL);

    printf("COLOR_YELLOW = 0x%x\n", COLOR_YELLOW);

    printf("calling display init\n");
    sdl_display_init(COLOR_YELLOW);

    printf("calling sdl_render_text\n");
    loc = sdl_render_text(100, 200, "hello world");
    printf("%d %d %d %d\n", 
       loc->x, loc->y, loc->w, loc->h);

    printf("calling sdl_render_printf\n");
    loc = sdl_render_printf(100, 400, "%s %d %d", "hello", 1, 2);
    printf("%d %d %d %d\n", 
       loc->x, loc->y, loc->w, loc->h);

    printf("calling display present\n");
    sdl_display_present();

    printf("sleep 3\n");
    sleep(3);

    printf("return\n");
    return 0;

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
