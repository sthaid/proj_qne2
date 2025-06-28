#include <common.h>

int SDL_main(int argc, char **argv)
{
    int  w, h;
    uint32_t color;
    double inten;

    INFO("STARTING\n");

    sdl_init(&w, &h);

    for (inten = .01; inten <= 1; inten += .01) {
        color = sdl_scale_color(COLOR_BLUE, inten);
        sdl_display_init(color);

        sdl_render_printf(0, 0, "%d %d", w, h);
        sdl_render_printf(0, h/2, "Q-%f", inten);

        sdl_display_present();
        usleep(100000);  // 100 ms
    }

    sdl_exit();

    INFO("TERMINATING\n");
    return 0;
}

