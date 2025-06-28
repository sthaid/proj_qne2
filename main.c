#include <common.h>

int SDL_main(int argc, char **argv)
{
    int i, w, h;

    INFO("STARTING\n");

    sdl_init(&w, &h);

    for (i = 30; i < 150; i++) {
#define COLOR_PURPLE     127,   0, 255, 255
        sdl_display_init(INTENSITY(COLOR_PURPLE,1,2,3,4));
        //sdl_display_init(INTENSITY(255,255,255,255,.25));

        sdl_render_printf(0, 0, 150, "%d %d", w, h, 1);  // xxx '1'
        sdl_render_printf(0, h/2, i, "Q-%d", i);

        sdl_display_present();
        usleep(100000);
    }

    sdl_exit();

    INFO("TERMINATING\n");
    return 0;
}

