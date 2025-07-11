#include <std_hdrs.h>

#include <sdl.h>

// xxx landscape

//
// logging
//

#define INFO(fmt, args...) \
    do { \
        logmsg("INFO", __func__, fmt, ## args); \
    } while (0)
#define ERROR(fmt, args...) \
    do { \
        logmsg("ERROR", __func__, fmt, ## args); \
    } while (0)

static void logmsg(char *lvl, const char *func, char *fmt, ...) 
    __attribute__ ((format (printf, 3, 4)));

//
// font defines
// 

#ifdef ANDROID
    #define FONT_FILE_PATH  "/system/fonts/DroidSansMono.ttf"
#else
    #define FONT_FILE_PATH "/usr/share/fonts/truetype/freefont/FreeMonoBold.ttf"
#endif

#define MAX_FONT_PTSIZE 200

//
// colors
//

#define BYTES_PER_PIXEL  4

const int COLOR_PURPLE     = ( 127  |    0<<8 |  255<<16 |  255<<24 );  // r,g,b,a
const int COLOR_BLUE       = ( 0    |    0<<8 |  255<<16 |  255<<24 );
const int COLOR_LIGHT_BLUE = ( 0    |  255<<8 |  255<<16 |  255<<24 );
const int COLOR_GREEN      = ( 0    |  255<<8 |    0<<16 |  255<<24 );
const int COLOR_YELLOW     = ( 255  |  255<<8 |    0<<16 |  255<<24 );
const int COLOR_ORANGE     = ( 255  |  128<<8 |    0<<16 |  255<<24 );
const int COLOR_PINK       = ( 255  |  105<<8 |  180<<16 |  255<<24 );
const int COLOR_RED        = ( 255  |    0<<8 |    0<<16 |  255<<24 );
const int COLOR_GRAY       = ( 224  |  224<<8 |  224<<16 |  255<<24 );
const int COLOR_WHITE      = ( 255  |  255<<8 |  255<<16 |  255<<24 );
const int COLOR_BLACK      = ( 0    |    0<<8 |    0<<16 |  255<<24 );

//
// typedefs
//

typedef struct {
    TTF_Font *font;
    //int       char_width;
    //int       char_height;
} font_t;

typedef struct {
    sdl_rect_t loc;
    int        event_id;
} event_t;

//
// variables
//

static SDL_Window     * window;
static SDL_Renderer   * renderer;
       int              win_width; //xxx
static int              win_height;

static font_t           font[MAX_FONT_PTSIZE];

static event_t          event_tbl[100];
static int              max_event;

//
// prototypes
//

static int process_sdl_event(SDL_Event *ev);
static void set_render_draw_color(int color);

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

    // initialize Simple DirectMedia Layer  (SDL)
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0) {  // xxx audio?
        ERROR("SDL_Init failed\n");
        return -1;
    }

    // create SDL Window and Renderer
#ifdef ANDROID
    if (SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN, &window, &renderer) != 0) {
        ERROR("SDL_CreateWindowAndRenderer failed\n");
        return -1;
    }
#else
    if (SDL_CreateWindowAndRenderer(500, 900, 0, &window, &renderer) != 0) {
        ERROR("SDL_CreateWindowAndRenderer failed\n");
        return -1;
    }
#endif

    // get the actual window size, which will be returned to caller and
    // also saved in vars win_width/height
    SDL_GetWindowSize(window, &win_width, &win_height);
    *w = win_width;
    *h = win_height;
    INFO("win_width=%d win_height=%d\n", win_width, win_height);

    // initialize True Type Font
    if (TTF_Init() < 0) {
        ERROR("TTF_Init failed\n");
        return -1;
    }
    sdl_print_init(40, COLOR_WHITE, COLOR_BLACK, NULL, NULL, NULL, NULL);

    // SDL Text Input is not being used 
    SDL_StopTextInput();

    // this is needed so that the first actual display present works
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

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    INFO("done\n");
}

// ----------------- DISPLAY INIT / PRESENT ---------------

void sdl_display_init(int color)
{
    max_event = 0;

    set_render_draw_color(color);
    SDL_RenderClear(renderer);
}

void sdl_display_present(void)
{
    SDL_RenderPresent(renderer);
}

// -----------------  EVENTS  -----------------------------

void sdl_register_event(sdl_rect_t *loc, int event_id)
{
    if (loc == NULL || loc->w == 0 || loc->h == 0) {
        ERROR("invalid loc, event_id=%d\n", event_id);
        return;
    }

    event_tbl[max_event].loc = *loc;
    event_tbl[max_event].event_id  = event_id; 
    max_event++;
}

// arg timeout_us:
//   -1:     wait forever
//    0:     don't wait
//    usecs: timeout
int sdl_get_event(long timeout_us)
{
    SDL_Event ev;
    int event_id = -1;
    int ret;
    long waited = 0;

try_again:
    // get event
    ret = SDL_PollEvent(&ev);

    // no event available, either return error or try again to get event
    if (ret == 0) {
        if (timeout_us == 0) {
            // dont wait
            return -1;
        } else if (timeout_us < 0 || waited < timeout_us) {
            // either wait forever or time waited is less than timeout_us
            usleep(100000);
            waited += 100000;
            goto try_again;
        } else {
            // time waited exceeds timeout_us
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

static void set_render_draw_color(int color)
{
    int r = (color >> 0) & 0xff;
    int g = (color >> 8) & 0xff;
    int b = (color >> 16) & 0xff;
    int a = (color >> 24) & 0xff;

    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

// -----------------  RENDER TEXT  ------------------------

int char_width_xxx; //xxx

static struct {
    int ptsize;
    SDL_Color fg_color;
    SDL_Color bg_color;
    int char_width;
    int char_height;
    int win_rows;
    int win_cols;
} text;

void sdl_print_init(int ptsize, int fg_color, int bg_color, int *char_width, int *char_height, int *win_rows, int *win_cols)
{
    int chw, chh;

    if (ptsize < 10) {
        ptsize = 10;
    }
    if (ptsize >= MAX_FONT_PTSIZE) {
        ptsize = MAX_FONT_PTSIZE-1;
    }

    if (font[ptsize].font == NULL) {
        font[ptsize].font = TTF_OpenFont(FONT_FILE_PATH, ptsize);
        if (font[ptsize].font == NULL) {
            ERROR("TTF_OpenFont failed, ptsize=%d\n", ptsize);
            return;
        }
    }
    TTF_SizeText(font[ptsize].font, "X", &chw, &chh);

    text.ptsize      = ptsize;
    text.fg_color    = *(SDL_Color*)&fg_color;
    text.bg_color    = *(SDL_Color*)&bg_color;
    text.char_width  = chw;
    text.char_height = chh;
    text.win_rows    = win_height / chh;
    text.win_cols    = win_width / chw;

    if (char_width) *char_width = text.char_width;
    if (char_height) *char_height = text.char_height;
    if (win_rows) *win_rows = text.win_rows;
    if (win_cols) *win_cols = text.win_cols;

    char_width_xxx = text.char_width; //xxx
}

sdl_rect_t *sdl_render_text(int x, int y, char * str)
{
    SDL_Surface    * surface;
    SDL_Texture    * texture;
    static SDL_Rect  pos;

    // if font not initialized then return error
    if (font[text.ptsize].font == NULL) {
        ERROR("font ptsize %d, not initialized\n", text.ptsize);
        memset(&pos, 0, sizeof(pos));
        return (sdl_rect_t*)&pos;
    }

    // render the string to a surface
    surface = TTF_RenderText_Shaded(font[text.ptsize].font, str, text.fg_color, text.bg_color);
    if (surface == NULL) {
        ERROR("TTF_RenderText_Shaded returned NULL\n");
        memset(&pos, 0, sizeof(pos));
        return (sdl_rect_t*)&pos;
    }

    // determine the display location
    pos.x = x;
    pos.y = y;
    pos.w = surface->w;
    pos.h = surface->h;

    // create texture from the surface, and render the texture
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_RenderCopy(renderer, texture, NULL, &pos);

    // clean up
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);

    // return the display location where the text was rendered
    return (sdl_rect_t*)&pos;
}

sdl_rect_t *sdl_render_printf(int x, int y, char * fmt, ...)
{
    char str[1000];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(str, sizeof(str), fmt, ap);
    va_end(ap);

    return sdl_render_text(x, y, str);
}

sdl_rect_t *sdl_render_printf_nk(int n, int k, int y, char * fmt, ...)
{
    char str[1000];
    va_list ap;
    int x;

    if (n == 0) {
        // xxx print error
        return sdl_render_text(0, y, str);
    }

    va_start(ap, fmt);
    vsnprintf(str, sizeof(str), fmt, ap);
    va_end(ap);

    x = ((win_width/2/(n)) + (k) * (win_width/(n)) - strlen(str) * text.char_width / 2);

    return sdl_render_text(x, y, str);
}

// -----------------  RENDER RECTANGLES, LINES, CIRCLES, POINTS  --------------------

void sdl_render_rect(sdl_rect_t *loc, int line_width, int color)
{
    SDL_Rect rect = *(SDL_Rect*)loc;
    int i;

    set_render_draw_color(color);

    for (i = 0; i < line_width; i++) {
        SDL_RenderDrawRect(renderer, &rect);
        if (rect.w < 2 || rect.h < 2) {
            break;
        }
        rect.x += 1;
        rect.y += 1;
        rect.w -= 2;
        rect.h -= 2;
    }
}

void sdl_render_fill_rect(sdl_rect_t *loc, int color)
{
    SDL_Rect rect = *(SDL_Rect*)loc;

    set_render_draw_color(color);
    SDL_RenderFillRect(renderer, &rect);
}

void sdl_render_line(int x1, int y1, int x2, int y2, int color)
{
    sdl_point_t points[2] = { {x1,y1}, {x2,y2} };
    sdl_render_lines(points, 2, color);
}

void sdl_render_lines(sdl_point_t *points, int count, int color)
{
    SDL_Point * sdl_points = (SDL_Point*)points;

    if (count <= 1) {
        return;
    }

    set_render_draw_color(color);

    SDL_RenderDrawLines(renderer, sdl_points, count);
}

void sdl_render_circle(int x_center, int y_center, int radius,
            int line_width, int color)
{
    int count = 0, i, angle, x, y;
    SDL_Point points[370];

    static int sin_table[370];
    static int cos_table[370];
    static bool first_call = true;

    // on first call make table of sin and cos indexed by degrees
    if (first_call) {
        for (angle = 0; angle < 362; angle++) {
            sin_table[angle] = sin(angle*(2*M_PI/360)) * (1<<10);
            cos_table[angle] = cos(angle*(2*M_PI/360)) * (1<<10);
        }
        first_call = false;
    }

    // set the color
    set_render_draw_color(color);

    // loop over line_width
    for (i = 0; i < line_width; i++) {
        // draw circle
        for (angle = 0; angle < 362; angle++) {
            x = x_center + ((radius * sin_table[angle]) >> 10);
            y = y_center + ((radius * cos_table[angle]) >> 10);
            points[count].x = x;
            points[count].y = y;
            count++;
        }
        SDL_RenderDrawLines(renderer, points, count);
        count = 0;

        // reduce radius by 1
        radius--;
        if (radius <= 0) {
            break;
        }
    }
}

void sdl_render_point(int x, int y, int color, int point_size)
{
    sdl_point_t point = {x,y};

    sdl_render_points(&point, 1, color, point_size);
}

void sdl_render_points(sdl_point_t *points, int count, int color, int point_size)
{
    #define MAX_SDL_POINTS 1000

    static struct point_extend_s {
        int max;
        struct point_extend_offset_s {
            int x;
            int y;
        } offset[300];
    } point_extend[10] = {
    { 1, {
        {0,0}, 
            } },
    { 5, {
        {-1,0}, 
        {0,-1}, {0,0}, {0,1}, 
        {1,0}, 
            } },
    { 21, {
        {-2,-1}, {-2,0}, {-2,1}, 
        {-1,-2}, {-1,-1}, {-1,0}, {-1,1}, {-1,2}, 
        {0,-2}, {0,-1}, {0,0}, {0,1}, {0,2}, 
        {1,-2}, {1,-1}, {1,0}, {1,1}, {1,2}, 
        {2,-1}, {2,0}, {2,1}, 
            } },
    { 37, {
        {-3,-1}, {-3,0}, {-3,1}, 
        {-2,-2}, {-2,-1}, {-2,0}, {-2,1}, {-2,2}, 
        {-1,-3}, {-1,-2}, {-1,-1}, {-1,0}, {-1,1}, {-1,2}, {-1,3}, 
        {0,-3}, {0,-2}, {0,-1}, {0,0}, {0,1}, {0,2}, {0,3}, 
        {1,-3}, {1,-2}, {1,-1}, {1,0}, {1,1}, {1,2}, {1,3}, 
        {2,-2}, {2,-1}, {2,0}, {2,1}, {2,2}, 
        {3,-1}, {3,0}, {3,1}, 
            } },
    { 61, {
        {-4,-1}, {-4,0}, {-4,1}, 
        {-3,-3}, {-3,-2}, {-3,-1}, {-3,0}, {-3,1}, {-3,2}, {-3,3}, 
        {-2,-3}, {-2,-2}, {-2,-1}, {-2,0}, {-2,1}, {-2,2}, {-2,3}, 
        {-1,-4}, {-1,-3}, {-1,-2}, {-1,-1}, {-1,0}, {-1,1}, {-1,2}, {-1,3}, {-1,4}, 
        {0,-4}, {0,-3}, {0,-2}, {0,-1}, {0,0}, {0,1}, {0,2}, {0,3}, {0,4}, 
        {1,-4}, {1,-3}, {1,-2}, {1,-1}, {1,0}, {1,1}, {1,2}, {1,3}, {1,4}, 
        {2,-3}, {2,-2}, {2,-1}, {2,0}, {2,1}, {2,2}, {2,3}, 
        {3,-3}, {3,-2}, {3,-1}, {3,0}, {3,1}, {3,2}, {3,3}, 
        {4,-1}, {4,0}, {4,1}, 
            } },
    { 89, {
        {-5,-1}, {-5,0}, {-5,1}, 
        {-4,-3}, {-4,-2}, {-4,-1}, {-4,0}, {-4,1}, {-4,2}, {-4,3}, 
        {-3,-4}, {-3,-3}, {-3,-2}, {-3,-1}, {-3,0}, {-3,1}, {-3,2}, {-3,3}, {-3,4}, 
        {-2,-4}, {-2,-3}, {-2,-2}, {-2,-1}, {-2,0}, {-2,1}, {-2,2}, {-2,3}, {-2,4}, 
        {-1,-5}, {-1,-4}, {-1,-3}, {-1,-2}, {-1,-1}, {-1,0}, {-1,1}, {-1,2}, {-1,3}, {-1,4}, {-1,5}, 
        {0,-5}, {0,-4}, {0,-3}, {0,-2}, {0,-1}, {0,0}, {0,1}, {0,2}, {0,3}, {0,4}, {0,5}, 
        {1,-5}, {1,-4}, {1,-3}, {1,-2}, {1,-1}, {1,0}, {1,1}, {1,2}, {1,3}, {1,4}, {1,5}, 
        {2,-4}, {2,-3}, {2,-2}, {2,-1}, {2,0}, {2,1}, {2,2}, {2,3}, {2,4}, 
        {3,-4}, {3,-3}, {3,-2}, {3,-1}, {3,0}, {3,1}, {3,2}, {3,3}, {3,4}, 
        {4,-3}, {4,-2}, {4,-1}, {4,0}, {4,1}, {4,2}, {4,3}, 
        {5,-1}, {5,0}, {5,1}, 
            } },
    { 121, {
        {-6,-1}, {-6,0}, {-6,1}, 
        {-5,-3}, {-5,-2}, {-5,-1}, {-5,0}, {-5,1}, {-5,2}, {-5,3}, 
        {-4,-4}, {-4,-3}, {-4,-2}, {-4,-1}, {-4,0}, {-4,1}, {-4,2}, {-4,3}, {-4,4}, 
        {-3,-5}, {-3,-4}, {-3,-3}, {-3,-2}, {-3,-1}, {-3,0}, {-3,1}, {-3,2}, {-3,3}, {-3,4}, {-3,5}, 
        {-2,-5}, {-2,-4}, {-2,-3}, {-2,-2}, {-2,-1}, {-2,0}, {-2,1}, {-2,2}, {-2,3}, {-2,4}, {-2,5}, 
        {-1,-6}, {-1,-5}, {-1,-4}, {-1,-3}, {-1,-2}, {-1,-1}, {-1,0}, {-1,1}, {-1,2}, {-1,3}, {-1,4}, {-1,5}, {-1,6}, 
        {0,-6}, {0,-5}, {0,-4}, {0,-3}, {0,-2}, {0,-1}, {0,0}, {0,1}, {0,2}, {0,3}, {0,4}, {0,5}, {0,6}, 
        {1,-6}, {1,-5}, {1,-4}, {1,-3}, {1,-2}, {1,-1}, {1,0}, {1,1}, {1,2}, {1,3}, {1,4}, {1,5}, {1,6}, 
        {2,-5}, {2,-4}, {2,-3}, {2,-2}, {2,-1}, {2,0}, {2,1}, {2,2}, {2,3}, {2,4}, {2,5}, 
        {3,-5}, {3,-4}, {3,-3}, {3,-2}, {3,-1}, {3,0}, {3,1}, {3,2}, {3,3}, {3,4}, {3,5}, 
        {4,-4}, {4,-3}, {4,-2}, {4,-1}, {4,0}, {4,1}, {4,2}, {4,3}, {4,4}, 
        {5,-3}, {5,-2}, {5,-1}, {5,0}, {5,1}, {5,2}, {5,3}, 
        {6,-1}, {6,0}, {6,1}, 
            } },
    { 177, {
        {-7,-2}, {-7,-1}, {-7,0}, {-7,1}, {-7,2}, 
        {-6,-4}, {-6,-3}, {-6,-2}, {-6,-1}, {-6,0}, {-6,1}, {-6,2}, {-6,3}, {-6,4}, 
        {-5,-5}, {-5,-4}, {-5,-3}, {-5,-2}, {-5,-1}, {-5,0}, {-5,1}, {-5,2}, {-5,3}, {-5,4}, {-5,5}, 
        {-4,-6}, {-4,-5}, {-4,-4}, {-4,-3}, {-4,-2}, {-4,-1}, {-4,0}, {-4,1}, {-4,2}, {-4,3}, {-4,4}, {-4,5}, {-4,6}, 
        {-3,-6}, {-3,-5}, {-3,-4}, {-3,-3}, {-3,-2}, {-3,-1}, {-3,0}, {-3,1}, {-3,2}, {-3,3}, {-3,4}, {-3,5}, {-3,6}, 
        {-2,-7}, {-2,-6}, {-2,-5}, {-2,-4}, {-2,-3}, {-2,-2}, {-2,-1}, {-2,0}, {-2,1}, {-2,2}, {-2,3}, {-2,4}, {-2,5}, {-2,6}, {-2,7}, 
        {-1,-7}, {-1,-6}, {-1,-5}, {-1,-4}, {-1,-3}, {-1,-2}, {-1,-1}, {-1,0}, {-1,1}, {-1,2}, {-1,3}, {-1,4}, {-1,5}, {-1,6}, {-1,7}, 
        {0,-7}, {0,-6}, {0,-5}, {0,-4}, {0,-3}, {0,-2}, {0,-1}, {0,0}, {0,1}, {0,2}, {0,3}, {0,4}, {0,5}, {0,6}, {0,7}, 
        {1,-7}, {1,-6}, {1,-5}, {1,-4}, {1,-3}, {1,-2}, {1,-1}, {1,0}, {1,1}, {1,2}, {1,3}, {1,4}, {1,5}, {1,6}, {1,7}, 
        {2,-7}, {2,-6}, {2,-5}, {2,-4}, {2,-3}, {2,-2}, {2,-1}, {2,0}, {2,1}, {2,2}, {2,3}, {2,4}, {2,5}, {2,6}, {2,7}, 
        {3,-6}, {3,-5}, {3,-4}, {3,-3}, {3,-2}, {3,-1}, {3,0}, {3,1}, {3,2}, {3,3}, {3,4}, {3,5}, {3,6}, 
        {4,-6}, {4,-5}, {4,-4}, {4,-3}, {4,-2}, {4,-1}, {4,0}, {4,1}, {4,2}, {4,3}, {4,4}, {4,5}, {4,6}, 
        {5,-5}, {5,-4}, {5,-3}, {5,-2}, {5,-1}, {5,0}, {5,1}, {5,2}, {5,3}, {5,4}, {5,5}, 
        {6,-4}, {6,-3}, {6,-2}, {6,-1}, {6,0}, {6,1}, {6,2}, {6,3}, {6,4}, 
        {7,-2}, {7,-1}, {7,0}, {7,1}, {7,2}, 
            } },
    { 221, {
        {-8,-2}, {-8,-1}, {-8,0}, {-8,1}, {-8,2}, 
        {-7,-4}, {-7,-3}, {-7,-2}, {-7,-1}, {-7,0}, {-7,1}, {-7,2}, {-7,3}, {-7,4}, 
        {-6,-5}, {-6,-4}, {-6,-3}, {-6,-2}, {-6,-1}, {-6,0}, {-6,1}, {-6,2}, {-6,3}, {-6,4}, {-6,5}, 
        {-5,-6}, {-5,-5}, {-5,-4}, {-5,-3}, {-5,-2}, {-5,-1}, {-5,0}, {-5,1}, {-5,2}, {-5,3}, {-5,4}, {-5,5}, {-5,6}, 
        {-4,-7}, {-4,-6}, {-4,-5}, {-4,-4}, {-4,-3}, {-4,-2}, {-4,-1}, {-4,0}, {-4,1}, {-4,2}, {-4,3}, {-4,4}, {-4,5}, {-4,6}, {-4,7}, 
        {-3,-7}, {-3,-6}, {-3,-5}, {-3,-4}, {-3,-3}, {-3,-2}, {-3,-1}, {-3,0}, {-3,1}, {-3,2}, {-3,3}, {-3,4}, {-3,5}, {-3,6}, {-3,7}, 
        {-2,-8}, {-2,-7}, {-2,-6}, {-2,-5}, {-2,-4}, {-2,-3}, {-2,-2}, {-2,-1}, {-2,0}, {-2,1}, {-2,2}, {-2,3}, {-2,4}, {-2,5}, {-2,6}, {-2,7}, {-2,8}, 
        {-1,-8}, {-1,-7}, {-1,-6}, {-1,-5}, {-1,-4}, {-1,-3}, {-1,-2}, {-1,-1}, {-1,0}, {-1,1}, {-1,2}, {-1,3}, {-1,4}, {-1,5}, {-1,6}, {-1,7}, {-1,8}, 
        {0,-8}, {0,-7}, {0,-6}, {0,-5}, {0,-4}, {0,-3}, {0,-2}, {0,-1}, {0,0}, {0,1}, {0,2}, {0,3}, {0,4}, {0,5}, {0,6}, {0,7}, {0,8}, 
        {1,-8}, {1,-7}, {1,-6}, {1,-5}, {1,-4}, {1,-3}, {1,-2}, {1,-1}, {1,0}, {1,1}, {1,2}, {1,3}, {1,4}, {1,5}, {1,6}, {1,7}, {1,8}, 
        {2,-8}, {2,-7}, {2,-6}, {2,-5}, {2,-4}, {2,-3}, {2,-2}, {2,-1}, {2,0}, {2,1}, {2,2}, {2,3}, {2,4}, {2,5}, {2,6}, {2,7}, {2,8}, 
        {3,-7}, {3,-6}, {3,-5}, {3,-4}, {3,-3}, {3,-2}, {3,-1}, {3,0}, {3,1}, {3,2}, {3,3}, {3,4}, {3,5}, {3,6}, {3,7}, 
        {4,-7}, {4,-6}, {4,-5}, {4,-4}, {4,-3}, {4,-2}, {4,-1}, {4,0}, {4,1}, {4,2}, {4,3}, {4,4}, {4,5}, {4,6}, {4,7}, 
        {5,-6}, {5,-5}, {5,-4}, {5,-3}, {5,-2}, {5,-1}, {5,0}, {5,1}, {5,2}, {5,3}, {5,4}, {5,5}, {5,6}, 
        {6,-5}, {6,-4}, {6,-3}, {6,-2}, {6,-1}, {6,0}, {6,1}, {6,2}, {6,3}, {6,4}, {6,5}, 
        {7,-4}, {7,-3}, {7,-2}, {7,-1}, {7,0}, {7,1}, {7,2}, {7,3}, {7,4}, 
        {8,-2}, {8,-1}, {8,0}, {8,1}, {8,2}, 
            } },
    { 277, {
        {-9,-2}, {-9,-1}, {-9,0}, {-9,1}, {-9,2}, 
        {-8,-4}, {-8,-3}, {-8,-2}, {-8,-1}, {-8,0}, {-8,1}, {-8,2}, {-8,3}, {-8,4}, 
        {-7,-6}, {-7,-5}, {-7,-4}, {-7,-3}, {-7,-2}, {-7,-1}, {-7,0}, {-7,1}, {-7,2}, {-7,3}, {-7,4}, {-7,5}, {-7,6}, 
        {-6,-7}, {-6,-6}, {-6,-5}, {-6,-4}, {-6,-3}, {-6,-2}, {-6,-1}, {-6,0}, {-6,1}, {-6,2}, {-6,3}, {-6,4}, {-6,5}, {-6,6}, {-6,7}, 
        {-5,-7}, {-5,-6}, {-5,-5}, {-5,-4}, {-5,-3}, {-5,-2}, {-5,-1}, {-5,0}, {-5,1}, {-5,2}, {-5,3}, {-5,4}, {-5,5}, {-5,6}, {-5,7}, 
        {-4,-8}, {-4,-7}, {-4,-6}, {-4,-5}, {-4,-4}, {-4,-3}, {-4,-2}, {-4,-1}, {-4,0}, {-4,1}, {-4,2}, {-4,3}, {-4,4}, {-4,5}, {-4,6}, {-4,7}, {-4,8}, 
        {-3,-8}, {-3,-7}, {-3,-6}, {-3,-5}, {-3,-4}, {-3,-3}, {-3,-2}, {-3,-1}, {-3,0}, {-3,1}, {-3,2}, {-3,3}, {-3,4}, {-3,5}, {-3,6}, {-3,7}, {-3,8}, 
        {-2,-9}, {-2,-8}, {-2,-7}, {-2,-6}, {-2,-5}, {-2,-4}, {-2,-3}, {-2,-2}, {-2,-1}, {-2,0}, {-2,1}, {-2,2}, {-2,3}, {-2,4}, {-2,5}, {-2,6}, {-2,7}, {-2,8}, {-2,9}, 
        {-1,-9}, {-1,-8}, {-1,-7}, {-1,-6}, {-1,-5}, {-1,-4}, {-1,-3}, {-1,-2}, {-1,-1}, {-1,0}, {-1,1}, {-1,2}, {-1,3}, {-1,4}, {-1,5}, {-1,6}, {-1,7}, {-1,8}, {-1,9}, 
        {0,-9}, {0,-8}, {0,-7}, {0,-6}, {0,-5}, {0,-4}, {0,-3}, {0,-2}, {0,-1}, {0,0}, {0,1}, {0,2}, {0,3}, {0,4}, {0,5}, {0,6}, {0,7}, {0,8}, {0,9}, 
        {1,-9}, {1,-8}, {1,-7}, {1,-6}, {1,-5}, {1,-4}, {1,-3}, {1,-2}, {1,-1}, {1,0}, {1,1}, {1,2}, {1,3}, {1,4}, {1,5}, {1,6}, {1,7}, {1,8}, {1,9}, 
        {2,-9}, {2,-8}, {2,-7}, {2,-6}, {2,-5}, {2,-4}, {2,-3}, {2,-2}, {2,-1}, {2,0}, {2,1}, {2,2}, {2,3}, {2,4}, {2,5}, {2,6}, {2,7}, {2,8}, {2,9}, 
        {3,-8}, {3,-7}, {3,-6}, {3,-5}, {3,-4}, {3,-3}, {3,-2}, {3,-1}, {3,0}, {3,1}, {3,2}, {3,3}, {3,4}, {3,5}, {3,6}, {3,7}, {3,8}, 
        {4,-8}, {4,-7}, {4,-6}, {4,-5}, {4,-4}, {4,-3}, {4,-2}, {4,-1}, {4,0}, {4,1}, {4,2}, {4,3}, {4,4}, {4,5}, {4,6}, {4,7}, {4,8}, 
        {5,-7}, {5,-6}, {5,-5}, {5,-4}, {5,-3}, {5,-2}, {5,-1}, {5,0}, {5,1}, {5,2}, {5,3}, {5,4}, {5,5}, {5,6}, {5,7}, 
        {6,-7}, {6,-6}, {6,-5}, {6,-4}, {6,-3}, {6,-2}, {6,-1}, {6,0}, {6,1}, {6,2}, {6,3}, {6,4}, {6,5}, {6,6}, {6,7}, 
        {7,-6}, {7,-5}, {7,-4}, {7,-3}, {7,-2}, {7,-1}, {7,0}, {7,1}, {7,2}, {7,3}, {7,4}, {7,5}, {7,6}, 
        {8,-4}, {8,-3}, {8,-2}, {8,-1}, {8,0}, {8,1}, {8,2}, {8,3}, {8,4}, 
        {9,-2}, {9,-1}, {9,0}, {9,1}, {9,2}, 
            } },
                };

    int i, j, x, y;
    SDL_Point sdl_points[MAX_SDL_POINTS];
    int sdl_points_count = 0;
    struct point_extend_s * pe = &point_extend[point_size];
    struct point_extend_offset_s * peo = pe->offset;

    if (count < 0) {
        return;
    }
    if (point_size < 0 || point_size > 9) {
        return;
    }

    set_render_draw_color(color);

    for (i = 0; i < count; i++) {
        for (j = 0; j < pe->max; j++) {
            x = points[i].x + peo[j].x;
            y = points[i].y + peo[j].y;
            sdl_points[sdl_points_count].x = x;
            sdl_points[sdl_points_count].y = y;
            sdl_points_count++;

            if (sdl_points_count == MAX_SDL_POINTS) {
                SDL_RenderDrawPoints(renderer, sdl_points, sdl_points_count);
                sdl_points_count = 0;
            }
        }
    }

    if (sdl_points_count > 0) {
        SDL_RenderDrawPoints(renderer, sdl_points, sdl_points_count);
        sdl_points_count = 0;
    }
}

// -----------------  RENDER USING TEXTURES  ---------------------------- 

sdl_texture_t *sdl_create_texture(int w, int h)
{
    SDL_Texture * texture;

    texture = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ABGR8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                w, h);
    if (texture == NULL) {
        ERROR("failed to allocate texture, %s\n", SDL_GetError());
        return NULL;
    }

    return (sdl_texture_t*)texture;
}

sdl_texture_t *sdl_create_texture_from_display(sdl_rect_t *loc)
{
    sdl_texture_t *texture;
    int ret;
    char * pixels;

    // allocate memory for the pixels
    pixels = calloc(1, loc->h * loc->w * BYTES_PER_PIXEL);
    if (pixels == NULL) {
        ERROR("allocate pixels failed\n");
        return NULL;
    }

    // read the pixels
    ret = SDL_RenderReadPixels(renderer, 
                               (SDL_Rect*)loc,
                               SDL_PIXELFORMAT_ABGR8888, 
                               pixels, 
                               loc->w * BYTES_PER_PIXEL);
    if (ret < 0) {
        ERROR("SDL_RenderReadPixels, %s\n", SDL_GetError());
        free(pixels);
        return NULL;
    }

    // create the texture
    texture = sdl_create_texture(loc->w, loc->h);
    if (texture == NULL) {
        ERROR("failed to allocate texture\n");
        free(pixels);
        return NULL;
    }

    // update the texture with the pixels
    SDL_UpdateTexture((SDL_Texture*)texture, NULL, pixels, loc->w * BYTES_PER_PIXEL);

    // free pixels
    free(pixels);

    // return the texture
    return texture;
}

sdl_texture_t *sdl_create_filled_circle_texture(int radius, int color)
{
    int width = 2 * radius + 1;
    int x = radius;
    int y = 0;
    int radiusError = 1-x;
    int pixels[width][width];
    sdl_texture_t * texture;

    #define DRAWLINE(Y, XS, XE, V) \
        do { \
            int i; \
            for (i = XS; i <= XE; i++) { \
                pixels[Y][i] = (V); \
            } \
        } while (0)

    // initialize pixels
    memset(pixels,0,sizeof(pixels));
    while(x >= y) {
        DRAWLINE(y+radius, -x+radius, x+radius, color);
        DRAWLINE(x+radius, -y+radius, y+radius, color);
        DRAWLINE(-y+radius, -x+radius, x+radius, color);
        DRAWLINE(-x+radius, -y+radius, y+radius, color);
        y++;
        if (radiusError<0) {
            radiusError += 2 * y + 1;
        } else {
            x--;
            radiusError += 2 * (y - x) + 1;
        }
    }

    // create the texture and copy the pixels to the texture
    texture = sdl_create_texture(width, width);
    if (texture == NULL) {
        ERROR("failed to allocate texture\n");
        return NULL;
    }
    SDL_SetTextureBlendMode((SDL_Texture*)texture, SDL_BLENDMODE_BLEND);
    SDL_UpdateTexture((SDL_Texture*)texture, NULL, pixels, width*BYTES_PER_PIXEL);

    // return texture
    return texture;
}

sdl_texture_t *sdl_create_text_texture(char * str)
{
    SDL_Surface * surface;
    SDL_Texture * texture;

    if (str[0] == '\0') {
        return NULL;
    }

    // if font not initialized then return error
    if (font[text.ptsize].font == NULL) {
        ERROR("font ptsize %d, not initialized\n", text.ptsize);
        return NULL;
    }

    // render the text to a surface,
    // create a texture from the surface
    // free the surface
    surface = TTF_RenderText_Shaded(font[text.ptsize].font, str, text.fg_color, text.bg_color);
    if (surface == NULL) {
        ERROR("failed to allocate surface\n");
        return NULL;
    }
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == NULL) {
        ERROR("failed to allocate texture\n");
        SDL_FreeSurface(surface);
        return NULL;
    }
    SDL_FreeSurface(surface);

    // return the texture which contains the text
    return (sdl_texture_t*)texture;
}

void sdl_destroy_texture(sdl_texture_t *texture)
{
    if (texture == NULL) {
        return;
    }

    SDL_DestroyTexture((SDL_Texture *)texture);
}

void sdl_query_texture(sdl_texture_t *texture, int * width, int * height)
{
    if (texture == NULL) {
        *width = 0;
        *height = 0;
        return;
    }

    SDL_QueryTexture((SDL_Texture *)texture, NULL, NULL, width, height);
}

void sdl_update_texture(sdl_texture_t *texture, char * pixels, int pitch)
{
    if (texture == NULL) {
        return;
    }

    SDL_UpdateTexture((SDL_Texture*)texture,
                      NULL,                   // update entire texture
                      pixels,                 // pixels
                      pitch);                 // pitch  
}

void sdl_render_texture(int x, int y, sdl_texture_t *texture)
{
    SDL_Rect dest;
    int w,h;

    if (texture == NULL) {
        return;
    }

    sdl_query_texture(texture, &w, &h);
    dest.x = x;
    dest.y = y;
    dest.w = w;
    dest.h = h;

    SDL_RenderCopy(renderer, (SDL_Texture*)texture, NULL, &dest);
}

void sdl_render_scaled_texture(sdl_rect_t *dest_arg, sdl_texture_t *texture_arg)
{
    SDL_Texture *texture = (SDL_Texture *)texture_arg;
    SDL_Rect *dest = (SDL_Rect*)dest_arg;

    if (texture == NULL) {
        return;
    }

    SDL_RenderCopy(renderer, texture, NULL, dest);
}

// -----------------  LOGGING  --------------------------------------

#define MAX_TIME_STR 50

static unsigned long get_real_time_us(void);
static char * time2str(char * str, long us, bool gmt, bool display_ms, bool display_date);

static void logmsg(char *lvl, const char *func, char *fmt, ...)
{
    va_list ap;
    char    msg[1000];
    char    time_str[MAX_TIME_STR]; 
    int     len;

    // construct msg
    va_start(ap, fmt);
    len = vsnprintf(msg, sizeof(msg), fmt, ap);  
    va_end(ap);

    // remove terminating newline
    if (len > 0 && msg[len-1] == '\n') {
        msg[len-1] = '\0';
        len--;
    }

    // print the message
    time2str(time_str, get_real_time_us(), false, true, true),
    fprintf(stderr, "%s %s %s: %s\n", time_str, lvl, func, msg);
}

static unsigned long get_real_time_us(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME,&ts);
    return ((unsigned long)ts.tv_sec * 1000000) + ((unsigned long)ts.tv_nsec / 1000);
}

static char * time2str(char * str, long us, bool gmt, bool display_ms, bool display_date)
{
    struct tm tm;
    time_t secs;
    int cnt;
    char * s = str;

    secs = us / 1000000;

    if (gmt) {
        gmtime_r(&secs, &tm);
    } else {
        localtime_r(&secs, &tm);
    }

    if (display_date) {
        cnt = sprintf(s, "%02d/%02d/%02d ",
                         tm.tm_mon+1, tm.tm_mday, tm.tm_year%100);
        s += cnt;
    }

    cnt = sprintf(s, "%02d:%02d:%02d",
                     tm.tm_hour, tm.tm_min, tm.tm_sec);
    s += cnt;

    if (display_ms) {
        cnt = sprintf(s, ".%03d", (int)((us % 1000000) / 1000));
        s += cnt;
    }

    if (gmt) {
        strcpy(s, " GMT");
    }

    return str;
}


