#include <common.h>

#include <SDL.h>
#include <SDL_ttf.h>

static SDL_Window     * sdl_window;
static SDL_Renderer   * sdl_renderer;
static int32_t          sdl_win_width;
static int32_t          sdl_win_height;

static TTF_Font       * font[MAX_FONT_PTSIZE];
static SDL_Color        text_fg_color;
static SDL_Color        text_bg_color;
static int32_t          text_ptsize;

// --------------------------------------------------------

int32_t sdl_init(int *w, int *h)
{
    // display available and current video drivers
    int num, i;
    num = SDL_GetNumVideoDrivers();
    INFO("Available Video Drivers: ");
    for (i = 0; i < num; i++) {
        INFO("   %s\n",  SDL_GetVideoDriver(i));
    }

    // xxx
    sdl_set_text_fg_color(COLOR_WHITE);
    sdl_set_text_bg_color(COLOR_BLACK);
    sdl_set_text_ptsize(100);

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

    // get the actual window size, which will be returned to caller and
    // also saved in vars sdl_win_width/height
    SDL_GetWindowSize(sdl_window, &sdl_win_width, &sdl_win_height);
    *w = sdl_win_width;
    *h = sdl_win_height;
    INFO("sdl_win_width=%d sdl_win_height=%d\n", sdl_win_width, sdl_win_height);

    // initialize True Type Font
    if (TTF_Init() < 0) {
        ERROR("TTF_Init failed\n");
        return -1;
    }

    // SDL Text Input is not being used 
    SDL_StopTextInput();

    // return success
    INFO("success\n");
    return 0;
}

void sdl_exit(void)
{
    int32_t i;

    for (i = 0; i < MAX_FONT_PTSIZE; i++) {
        if (font[i] != NULL) {
            TTF_CloseFont(font[i]);
        }
    }
    TTF_Quit();

    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
}

// --------------------------------------------------------

void sdl_display_init(uint32_t color)
{
    sdl_set_render_draw_color(color);

    SDL_RenderClear(sdl_renderer);
}

void sdl_display_present(void)
{
    SDL_RenderPresent(sdl_renderer);
}

// -----------------  COLORS  -----------------------------

uint32_t sdl_create_color(int r, int g, int b, int a)
{
    return (r << 0) | (g << 8) | (b << 16) | (a << 24);
}

uint32_t sdl_scale_color(uint32_t color, double inten)
{
    uint32_t r = (color >> 0) & 0xff;
    uint32_t g = (color >> 8) & 0xff;
    uint32_t b = (color >> 16) & 0xff;
    uint32_t a = (color >> 24) & 0xff;

    if (inten < 0) inten = 0;
    if (inten > 1) inten = 1;

    r *= inten;
    g *= inten;
    b *= inten;

    return (r << 0) | (g << 8) | (b << 16) | (a << 24);
}


void sdl_set_render_draw_color(uint32_t color)
{
    uint32_t r = (color >> 0) & 0xff;
    uint32_t g = (color >> 8) & 0xff;
    uint32_t b = (color >> 16) & 0xff;
    uint32_t a = (color >> 24) & 0xff;

    SDL_SetRenderDrawColor(sdl_renderer, r, g, b, a);
}

// -----------------  RENDER TEXT  ------------------------

void sdl_set_text_ptsize(int32_t ptsize)
{
    if (ptsize < 50) ptsize = 50;
    if (ptsize >= MAX_FONT_PTSIZE) ptsize = MAX_FONT_PTSIZE-1;

    text_ptsize = ptsize;
}

void sdl_set_text_fg_color(uint32_t color)
{
    text_fg_color = *(SDL_Color*)&color;
}

void sdl_set_text_bg_color(uint32_t color)
{
    text_bg_color = *(SDL_Color*)&color;
}

void sdl_render_text(int32_t x, int32_t y, char * str)
{
    SDL_Surface    * surface;
    SDL_Texture    * texture;
    SDL_Rect         pos;

    // xxx
    if (font[text_ptsize] == NULL) {
        font[text_ptsize] = TTF_OpenFont("/system/fonts/DroidSansMono.ttf", text_ptsize);
        if (font[text_ptsize] == NULL) {
            FATAL("TTF_OpenFont failed, text_ptsize=%d\n", text_ptsize);
        }
    }

    // render the string to a surface
    surface = TTF_RenderText_Shaded(font[text_ptsize], str, text_fg_color, text_bg_color);
    if (surface == NULL) {
        FATAL("TTF_RenderText_Shaded returned NULL\n");
    }

    // determine the display location
    pos.x = x;
    pos.y = y;
    pos.w = surface->w;
    pos.h = surface->h;

    // create texture from the surface, and render the texture
    texture = SDL_CreateTextureFromSurface(sdl_renderer, surface);
    SDL_RenderCopy(sdl_renderer, texture, NULL, &pos);

    // clean up
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void sdl_render_printf(int32_t x, int32_t y, char * fmt, ...)
{
    char str[1000];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(str, sizeof(str), fmt, ap);
    va_end(ap);

    sdl_render_text(x, y, str);
}
