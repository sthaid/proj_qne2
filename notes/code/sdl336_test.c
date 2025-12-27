#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#define INFO(fmt, args...) \
    do { \
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "INFO " fmt, ## args); \
    } while (0)
#define ERROR(fmt, args...) \
    do { \
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "ERROR " fmt, ## args); \
    } while (0)

#ifdef ANDROID
#define FONT_FILE_PATH "/system/fonts/DroidSansMono.ttf"
#else
#define FONT_FILE_PATH "/usr/share/fonts/truetype/freefont/FreeMonoBold.ttf"
#endif

SDL_Window           *window;
static SDL_Renderer  *renderer;
TTF_Font             *font;

void init(void)
{
    INFO("initializing\n");

    INFO("SDL Version = %d\n", SDL_GetVersion());

    // initialize SDL video
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        ERROR("SDL_Init VIDEO failed, %s\n", SDL_GetError());
        exit(1);   
    }

    // create SDL Window and Renderer
#ifdef ANDROID
    if (!SDL_CreateWindowAndRenderer("ezApp", 0, 0, SDL_WINDOW_FULLSCREEN, &window, &renderer)) {
        ERROR("SDL_CreateWindowAndRenderer failed\n");
        exit(1);
    }
#else
    if (!SDL_CreateWindowAndRenderer("ezApp", 800, 800, 0, &window, &renderer)) {
        ERROR("SDL_CreateWindowAndRenderer failed\n");
        exit(1);
    }
#endif

    // initialize font
    if (!TTF_Init()) {
        ERROR("TTF_Init failed\n");
        exit(1);
    }
    font = TTF_OpenFont(FONT_FILE_PATH, 100);
    if (font == NULL) {
        ERROR("TTF_OpenFont failed\n");
        exit(1);
    }

    // enable alpha blending
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
}

void render_text(int x, int y, char * str)
{
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_FRect     pos;

    SDL_Color blue = {0,0,255,255};
    SDL_Color black = {0,0,0,0};

    // render the string to a surface
    surface = TTF_RenderText_Shaded(font, str, 0, blue, black);
    if (surface == NULL) {
        ERROR("TTF_RenderText_Shaded returned NULL\n");
        exit(1);
    }

    // create texture from the surface, and render the texture
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    pos.x = x;
    pos.y = y;
    pos.w = surface->w;
    pos.h = surface->h;
    SDL_RenderTexture(renderer, texture, NULL, &pos);

    // clean up
    SDL_DestroySurface(surface);
    SDL_DestroyTexture(texture);
}

#ifdef ANDROID
int SDL_main()
#else
int main()
#endif
{
    // init window, renderer and font
    init();

    // create texture, and init it to white
    SDL_Texture *texture;
    texture = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ABGR8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                100, 100);
    unsigned int pixels[100][100];
    memset(pixels, 0xff, sizeof(pixels));
    SDL_UpdateTexture(texture, NULL, pixels, 100*4);

    // clear backbuffer to red
    SDL_SetRenderDrawColor(renderer, 0xff, 0x00, 0x00, 0xff);  // r, g, b, a
    SDL_RenderClear(renderer);

    // render 100x100 white square texture at top left
    SDL_FRect dest = {50,50,100,100};
    SDL_RenderTexture(renderer, texture, NULL, &dest);

    // render text
    // PROBLEM: the 'X' appears on the display in 2 places:
    //          - where it should, at 400x400
    //          - within the texture, located near the top left
    render_text(400, 400, "X");

    // present display and short sleep before terminating
    SDL_RenderPresent(renderer);
    sleep(10);

    return 0;
}
