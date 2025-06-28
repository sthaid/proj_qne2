#include <common.h>

int SDL_main(int argc, char **argv)
{
    INFO("STARTING\n");

    sdl_init(false);

    sleep(5);

    sdl_exit();

    INFO("TERMINATING\n");
    return 0;
}
