#include <common.h>

#include <SDL.h>
#include <SDL_ttf.h>

typedef struct {
    TTF_Font * font;
    int32_t    char_width;
    int32_t    char_height;
} sdl_font_t;




static SDL_Window     * sdl_window;
static SDL_Renderer   * sdl_renderer;

static int32_t          sdl_win_width;
static int32_t          sdl_win_height;

static sdl_font_t       sdl_font[MAX_FONT_PTSIZE];
static char           * sdl_font_path;

// --------------------------------------------------------

int32_t sdl_init(bool swap_white_black)
{
    // display available and current video drivers
    int num, i;
    num = SDL_GetNumVideoDrivers();
    INFO("Available Video Drivers: ");
    for (i = 0; i < num; i++) {
        INFO("   %s\n",  SDL_GetVideoDriver(i));
    }

    // initialize Simple DirectMedia Layer  (SDL)
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0) {  // xxx audio?
        ERROR("SDL_Init failed\n");
        return -1;
    }

    // create SDL Window and Renderer
    if (SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN, &sdl_window, &sdl_renderer) != 0) {
        ERROR("SDL_CreateWindowAndRenderer failed\n");
        return -1;
    }

#if 0
    // xxx what is this for
    for (i = 0; i < 50; i++) {
        sdl_event_t *event;
        event = sdl_poll_event();
        if (event->event_id == SDL_EVENT_WIN_SIZE_CHANGE) {
            INFO("got SDL_EVENT_WIN_SIZE_CHANGE, i = %d\n", i);
            break;
        }
        usleep(10000);
    }
#endif

    // get the actual window size, which will be returned to caller and
    // also saved in vars sdl_win_width/height
    SDL_GetWindowSize(sdl_window, &sdl_win_width, &sdl_win_height);
    INFO("sdl_win_width=%d sdl_win_height=%d\n", sdl_win_width, sdl_win_height);

    // initialize True Type Font
    if (TTF_Init() < 0) {
        ERROR("TTF_Init failed\n");
        return -1;
    }

    // determine sdl_font_path by searching for FreeMonoBold.ttf font file in possible locations
    // note - fonts can be installed using:
    //   sudo yum install gnu-free-mono-fonts       # rhel,centos,fedora
    //   sudo apt-get install fonts-freefont-ttf    # raspberrypi, ubuntu
    sdl_font_path = "/system/fonts/DroidSansMono.ttf";
    // xxx check it exits
    INFO("using font %s\n", sdl_font_path);

    // SDL Text Input is not being used 
    SDL_StopTextInput();

#if 0 // xxx later
    // if caller requests swap_white_black then swap the white and black
    // entries of the sdl_color_to_rgba table
    if (swap_white_black) {
        uint32_t tmp = sdl_color_to_rgba[SDL_WHITE];
        sdl_color_to_rgba[SDL_WHITE] = sdl_color_to_rgba[SDL_BLACK];
        sdl_color_to_rgba[SDL_BLACK] = tmp;
    }

    // register exit handler
    // xxx is this needed   atexit(exit_handler);
#endif

    // return success
    INFO("success\n");
    return 0;
}


void sdl_exit(void)
{
    int32_t i;

    for (i = 0; i < MAX_FONT_PTSIZE; i++) {
        if (sdl_font[i].font != NULL) {
            TTF_CloseFont(sdl_font[i].font);
        }
    }
    TTF_Quit();

    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
}

// --------------------------------------------------------


void sdl_render_printf(int32_t x, int32_t y, int32_t font_ptsize, char * fmt, ...)
{
    char str[1000];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(str, sizeof(str), fmt, ap);
    va_end(ap);

    sdl_render_text(x, y, font_ptsize, str);
}


    // render the text to a surface
    fg_color = (mouse_event != MOUSE_EVENT_NONE ? fg_color_event : fg_color_normal);
    surface = TTF_RenderText_Shaded(font.font, s, fg_color, bg_color);
    if (surface == NULL) {
        FATAL("TTF_RenderText_Shaded returned NULL\n");
    }

    // determine the display location
    pos.x = pane->x + col * font.char_width;
    pos.y = pane->y + row * font.char_height;
    pos.w = surface->w;
    pos.h = surface->h;

    // create texture from the surface, and render the texture
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_RenderCopy(renderer, texture, NULL, &pos);

    // clean up
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);

    // if there is a mouse_event then save the location for the event handler
    if (mouse_event != MOUSE_EVENT_NONE) {
        event.mouse_event_pos[mouse_event] = pos;
    }


rect_t sdl_render_text(rect_t * pane, int32_t x, int32_t y, int32_t font_ptsize, char * str, 
                       int32_t fg_color, int32_t bg_color)
{
    texture_t texture;
    int32_t   width, height;
    rect_t    loc, loc_clipped = {0,0,0,0};

    // if zero length string just return
    if (str[0] == '\0') {
        return loc_clipped;
    }

    // create the text texture
    texture =  sdl_create_text_texture(fg_color, bg_color, font_ptsize, str);
    if (texture == NULL) {
        ERROR("sdl_create_text_texture failed\n");
        return loc_clipped;
    }
    sdl_query_texture(texture, &width, &height);

    // determine the location within the pane that this
    // texture is to be rendered
    loc.x = x;
    loc.y = y;
    loc.w = width;
    loc.h = height;

    // render the texture
    loc_clipped = sdl_render_scaled_texture(pane, &loc, texture);

    // clean up
    sdl_destroy_texture(texture);

    // return the location of the text (possibly clipped), within the pane
    return loc_clipped;
}



