#include <common.h>

#include <SDL.h>
#include <SDL_ttf.h>

//typedef struct {
//    TTF_Font * font;
//    int32_t    char_width;
//    int32_t    char_height;
//} sdl_font_t;

static SDL_Window     * sdl_window;
static SDL_Renderer   * sdl_renderer;

static int32_t          sdl_win_width;
static int32_t          sdl_win_height;

//static sdl_font_t       sdl_font[MAX_FONT_PTSIZE];
//static char           * sdl_font_path;

static TTF_Font * font[MAX_FONT_PTSIZE];

static SDL_Color text_fg_color = {COLOR_WHITE};
static SDL_Color text_bg_color = {COLOR_BLACK};

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

void sdl_display_init(int r, int g, int b, int a)
{
    sdl_set_render_draw_color(r, g, b, a);

    SDL_RenderClear(sdl_renderer);
}

void sdl_display_present(void)
{
    SDL_RenderPresent(sdl_renderer);
}

// -----------------  COLORS  -----------------------------

void sdl_set_render_draw_color(int r, int g, int b, int a)
{
    SDL_SetRenderDrawColor(sdl_renderer, r, g, b, a);
}

void sdl_set_text_fg_color(int r, int g, int b, int a)
{
    text_fg_color = (SDL_Color){r, g, b, a};
}
    
void sdl_set_text_bg_color(int r, int g, int b, int a)
{
    text_bg_color = (SDL_Color){r, g, b, a};
}

// -----------------  RENDER TEXT  ------------------------

void sdl_render_string(int32_t x, int32_t y, int32_t font_ptsize, char * str)
{
    SDL_Surface    * surface;
    SDL_Texture    * texture;
    SDL_Rect         pos;

    // xxx
    if (font[font_ptsize] == NULL) {
        font[font_ptsize] = TTF_OpenFont("/system/fonts/DroidSansMono.ttf", font_ptsize);
        if (font[font_ptsize] == NULL) {
            FATAL("TTF_OpenFont failed, font_ptsize=%d\n", font_ptsize);
        }
    }

    // render the string to a surface
    surface = TTF_RenderText_Shaded(font[font_ptsize], str, text_fg_color, text_bg_color);
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

void sdl_render_printf(int32_t x, int32_t y, int32_t font_ptsize, char * fmt, ...)
{
    char str[1000];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(str, sizeof(str), fmt, ap);
    va_end(ap);

    sdl_render_string(x, y, font_ptsize, str);
}
