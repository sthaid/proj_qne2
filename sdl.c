#include <std_hdrs.h>
#include <sdl.h>
#include <utils.h>

// xxx landscape

typedef struct {
    TTF_Font *font;
    int   char_width;
    int   char_height;
} sdl_font_t;  // xxx rename

typedef struct {
    struct sdl_rect loc;
    int event_id;
} event_t;

static SDL_Window     * sdl_window;  // xxx rename
static SDL_Renderer   * sdl_renderer;
static int          sdl_win_width;
static int          sdl_win_height;

static sdl_font_t       font[MAX_FONT_PTSIZE];
static SDL_Color        text_fg_color;
static SDL_Color        text_bg_color;
static int          text_ptsize;

static event_t          event_tbl[100];
static int              max_event;

const int COLOR_YELLOW  =  ( 255  |  255<<8 |    0<<16 |  255<<24 );


// ----------------- INIT / EXIT --------------------------

int sdl_init(int *w, int *h)
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

    // xxx
    sdl_display_init(COLOR_BLACK);
    sdl_display_present();

    // return success
    INFO("success\n");
    return 0;
}

void sdl_exit(void)
{
    int i;

    INFO("sdl exitting\n");

    for (i = 0; i < MAX_FONT_PTSIZE; i++) {
        if (font[i].font != NULL) {
            TTF_CloseFont(font[i].font);
        }
    }
    TTF_Quit();

    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();

    INFO("done\n");
}

// ----------------- DISPLAY INIT / PRESENT ---------------

void sdl_display_init(int color)
{
    max_event = 0;

    sdl_set_render_draw_color(color);

    SDL_RenderClear(sdl_renderer);
}

void sdl_display_present(void)
{
    SDL_RenderPresent(sdl_renderer);
}

// -----------------  COLORS  -----------------------------

int sdl_create_color(int r, int g, int b, int a)
{
    return (r << 0) | (g << 8) | (b << 16) | (a << 24);
}

int sdl_scale_color(int color, double inten)
{
    int r = (color >> 0) & 0xff;
    int g = (color >> 8) & 0xff;
    int b = (color >> 16) & 0xff;
    int a = (color >> 24) & 0xff;

    if (inten < 0) inten = 0;
    if (inten > 1) inten = 1;

    r *= inten;
    g *= inten;
    b *= inten;

    return (r << 0) | (g << 8) | (b << 16) | (a << 24);
}

void sdl_set_render_draw_color(int color)
{
    int r = (color >> 0) & 0xff;
    int g = (color >> 8) & 0xff;
    int b = (color >> 16) & 0xff;
    int a = (color >> 24) & 0xff;

    SDL_SetRenderDrawColor(sdl_renderer, r, g, b, a);
}

// -----------------  PRINT TEXT  -------------------------

static void font_init(void);

void sdl_set_text_ptsize(int ptsize)
{
    if (ptsize < 50) ptsize = 50;
    if (ptsize >= MAX_FONT_PTSIZE) ptsize = MAX_FONT_PTSIZE-1;

    text_ptsize = ptsize;
}

void sdl_set_text_fg_color(int color)
{
    text_fg_color = *(SDL_Color*)&color;
}

void sdl_set_text_bg_color(int color)
{
    text_bg_color = *(SDL_Color*)&color;
}

// xxx return the rect
struct sdl_rect *sdl_render_text(int x, int y, char * str) // xxx name
{
    SDL_Surface    * surface;
    SDL_Texture    * texture;
    SDL_Rect         pos;
    static struct sdl_rect       pos2;

    // xxx
    font_init();

    // render the string to a surface
    surface = TTF_RenderText_Shaded(font[text_ptsize].font, str, text_fg_color, text_bg_color);
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

    // xxx
    pos2.x = pos.x;
    pos2.y = pos.y;
    pos2.w = pos.w;
    pos2.h = pos.h;
    return &pos2;  // xxx check this
}

struct sdl_rect *sdl_render_printf(int x, int y, char * fmt, ...)
{
    char str[1000];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(str, sizeof(str), fmt, ap);
    va_end(ap);

    return sdl_render_text(x, y, str);
}

void sdl_get_char_size(int *char_width, int *char_height)
{
    font_init();
    *char_width = font[text_ptsize].char_width;
    *char_height = font[text_ptsize].char_height;
}

static void font_init(void)
{
    if (font[text_ptsize].font == NULL) {
        font[text_ptsize].font = TTF_OpenFont("/system/fonts/DroidSansMono.ttf", text_ptsize);
        if (font[text_ptsize].font == NULL) {
            FATAL("TTF_OpenFont failed, text_ptsize=%d\n", text_ptsize);  // xxx use of FATAL
        }

        TTF_SizeText(font[text_ptsize].font, "X", &font[text_ptsize].char_width, &font[text_ptsize].char_height);
        INFO("text_ptsize=%d char_width=%d char_height=%d\n",
              text_ptsize, font[text_ptsize].char_width, font[text_ptsize].char_height);
    }
}

// -----------------  XXX EVENTS   ------------------------

static int process_sdl_event(SDL_Event *ev);

void sdl_register_event(struct sdl_rect *loc, int event_id)
{
    event_tbl[max_event].loc = *loc;
    event_tbl[max_event].event_id  = event_id; 
    max_event++;
}

int sdl_get_event(bool wait)
{
    SDL_Event ev;
    int event_id = -1;
    int ret;

try_again:
    // get event
    ret = SDL_PollEvent(&ev);

    // if no sdl event then, depending on 'wait' arg, either
    // sleep 100ms and try again, or return -1
    if (ret == 0) {
        if (wait) {
            usleep(100000);
            goto try_again;
        } else {
            return -1;
        }
    }

    // process the sdl_event; this may or may not return an event_id
    event_id = process_sdl_event(&ev);
    if (event_id == -1) {
        goto try_again;
    }

    // got an event_id, return it
    return event_id;
}

static int process_sdl_event(SDL_Event *ev)
{
    #define AT_POS(X,Y,pos) (((X) >= (pos).x) && \
                             ((X) < (pos).x + (pos).w) && \
                             ((Y) >= (pos).y) && \
                             ((Y) < (pos).y + (pos).h))

    int event_id = -1;
    int i;

    switch (ev->type) {
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: {
       INFO("MOUSEBUTTON button=%s state=%s x=%d y=%d\n",
               (ev->button.button == SDL_BUTTON_LEFT   ? "LEFT" :
                ev->button.button == SDL_BUTTON_MIDDLE ? "MIDDLE" :
                ev->button.button == SDL_BUTTON_RIGHT  ? "RIGHT" : "???"),
               (ev->button.state == SDL_PRESSED  ? "PRESSED" :
                ev->button.state == SDL_RELEASED ? "RELEASED" : "???"),
               ev->button.x,
               ev->button.y);

        if (ev->button.state == SDL_RELEASED) {
            for (i = 0; i < max_event; i++) {
                if (AT_POS(ev->button.x, ev->button.y, event_tbl[i].loc)) {
                    break;
                }
            }
            if (i < max_event) {
                event_id = event_tbl[i].event_id;
            }
        }
        break; }
    case SDL_MOUSEMOTION: {
#if 0
        INFO("MOUSEMOTION state=%s x=%d y=%d xrel=%d yrel=%d\n",
               (ev->motion.state == SDL_PRESSED  ? "PRESSED" :
                ev->motion.state == SDL_RELEASED ? "RELEASED" : "???"),
               ev->motion.x,
               ev->motion.y,
               ev->motion.xrel,
               ev->motion.yrel);
#endif
        break; }
    case SDL_FINGERDOWN:  // xxx should these be used instead of mouse
    case SDL_FINGERUP:
    case SDL_FINGERMOTION: {
        break; }
    default: {
        INFO("event_type %d - not supported\n", ev->type);
        break; }
    }

    return event_id;
}

#if 0
typedef struct SDL_TouchFingerEvent
{
    Uint32 type;        /**< SDL_FINGERMOTION or SDL_FINGERDOWN or SDL_FINGERUP */
    Uint32 timestamp;   /**< In milliseconds, populated using SDL_GetTicks() */
    SDL_TouchID touchId; /**< The touch device id */
    SDL_FingerID fingerId;
    float x;            /**< Normalized in the range 0...1 */
    float y;            /**< Normalized in the range 0...1 */
    float dx;           /**< Normalized in the range -1...1 */
    float dy;           /**< Normalized in the range -1...1 */
    float pressure;     /**< Normalized in the range 0...1 */
    Uint32 windowID;    /**< The window underneath the finger, if any */
} SDL_TouchFingerEvent;

typedef struct SDL_MouseButtonEvent
{
    Uint32 type;        /**< SDL_MOUSEBUTTONDOWN or SDL_MOUSEBUTTONUP */
    Uint32 timestamp;   /**< In milliseconds, populated using SDL_GetTicks() */
    Uint32 windowID;    /**< The window with mouse focus, if any */
    Uint32 which;       /**< The mouse instance id, or SDL_TOUCH_MOUSEID */
    Uint8 button;       /**< The mouse button index */
    Uint8 state;        /**< SDL_PRESSED or SDL_RELEASED */
    Uint8 clicks;       /**< 1 for single-click, 2 for double-click, etc. */
    Uint8 padding1;
    Sint32 x;           /**< X coordinate, relative to window */
    Sint32 y;           /**< Y coordinate, relative to window */
} SDL_MouseButtonEvent;

typedef struct SDL_MouseMotionEvent
{
    Uint32 type;        /**< SDL_MOUSEMOTION */
    Uint32 timestamp;   /**< In milliseconds, populated using SDL_GetTicks() */
    Uint32 windowID;    /**< The window with mouse focus, if any */
    Uint32 which;       /**< The mouse instance id, or SDL_TOUCH_MOUSEID */
    Uint32 state;       /**< The current button state */
    Sint32 x;           /**< X coordinate, relative to window */
    Sint32 y;           /**< Y coordinate, relative to window */
    Sint32 xrel;        /**< The relative motion in the X direction */
    Sint32 yrel;        /**< The relative motion in the Y direction */
} SDL_MouseMotionEvent;
#endif
