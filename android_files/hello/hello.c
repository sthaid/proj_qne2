#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include <sdl.h>

int main(int argc, char **argv)
{
    time_t t;
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

    printf("calling display init\n");
    sdl_display_init(COLOR_YELLOW);

    printf("calling sdl_render_text\n");
    loc = sdl_render_text(100, 200, "hello world");
    printf("%d %d %d %d\n", loc->x, loc->y, loc->w, loc->h);

    printf("calling sdl_render_printf\n");
    loc = sdl_render_printf(100, 400, "%s %d %d", "hello", 1, 2);
    printf("%d %d %d %d\n", loc->x, loc->y, loc->w, loc->h);

    printf("calling display present\n");
    sdl_display_present();

    printf("sleep 3\n");
    sleep(3);

    printf("return\n");
    return 0;
}
