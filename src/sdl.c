#include <std_hdrs.h>

#include <sdl.h>
#include <logging.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

// xxx landscape
// xxx keyboard events for < > END and up/down
// xxx read pixels routien
// xxx use of rint vs nearbyint

// xxx try SDL_SetRenderLogicalPresentation

//
// logging
//

//
// font defines
// 

#define FONT_FILE_PATH  "FreeMonoBold.ttf"

#define DEFAULT_FONTSZ 20

#define MIN_FONT_PTSIZE  10
#define MAX_FONT_PTSIZE  200

#define ONE_MS 1000
#define TEN_MS 10000

//
// typedefs
//

typedef struct {
    sdl_loc_t loc;
    int       event_id;
} event_t;

//
// global variables
//

int sdl_win_width;
int sdl_win_height;
int sdl_char_width;
int sdl_char_height;

//
// variables
//

static SDL_Window     * window;
static SDL_Renderer   * renderer;
static double           scale;

static TTF_Font        *font[MAX_FONT_PTSIZE];

static event_t          event_tbl[100];
static int              max_event;
static bool             evid_swipe_right_registered;
static bool             evid_swipe_left_registered;
static bool             evid_motion_registered;
static bool             evid_keybd_registered;

//
// prototypes
//

static void process_sdl_event(SDL_Event *ev, sdl_event_t *event);
static void set_render_draw_color(int color);

// ----------------- INIT / EXIT --------------------------

int sdl_init(void)
{
    int real_win_width, real_win_height;
    int num, i;
    double aspect_ratio;

    // display available and current video drivers
    num = SDL_GetNumVideoDrivers();
    INFO("Available Video Drivers: ");
    for (i = 0; i < num; i++) {
        INFO("   %s\n",  SDL_GetVideoDriver(i));
    }

    // initialize Simple DirectMedia Layer  (SDL)
    if (!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_SENSOR)) {
        ERROR("SDL_Init failed\n");
        return -1;
    }

    // create SDL Window and Renderer
#ifdef ANDROID
    if (!SDL_CreateWindowAndRenderer("xxxxx", 0, 0, SDL_WINDOW_FULLSCREEN, &window, &renderer)) {
        ERROR("SDL_CreateWindowAndRenderer failed\n");
        return -1;
    }
#else
    // xxx test with larger win width
    if (!SDL_CreateWindowAndRenderer("xxxxx", 450, 975, 0, &window, &renderer)) {
        ERROR("SDL_CreateWindowAndRenderer failed\n");
        return -1;
    }
#endif

    // get real windows size and aspect ratio
    SDL_GetWindowSize(window, &real_win_width, &real_win_height);
    aspect_ratio = (double)real_win_height / real_win_width;
    INFO("real win_width x height = %d %d  aspect = %f\n", real_win_width, real_win_height, aspect_ratio);

    // xxx
    sdl_win_width  = 1000;
    sdl_win_height = rint(1000 * aspect_ratio);
    scale = (double)real_win_width / sdl_win_width;
    INFO("logical sdl_win_width x height = %d %d  scale = %f\n", sdl_win_width, sdl_win_height, scale);

    // initialize True Type Font
    if (!TTF_Init()) {
        ERROR("TTF_Init failed\n");
        return -1;
    }

#if 0 //xxx make this a font routine
    // debug code to print font info
    for (int ptsize = MIN_FONT_PTSIZE; ptsize < MAX_FONT_PTSIZE; ptsize++) {
        TTF_Font *f = TTF_OpenFont(FONT_FILE_PATH, ptsize);
        TTF_SizeText(f, "X", &chw, &chh);
        TTF_CloseFont(f);
        INFO("font ptsize = %d  chw/chh = %d %d\n", ptsize, chw, chh);
    }
#endif

    // init default fontsize, where DEFAULT_FONTSZ is num chars across display;
    // and validate expected character size and columns
    sdl_print_init(DEFAULT_FONTSZ, COLOR_WHITE, COLOR_BLACK);
    INFO("sdl_print_init(%d) sdl_char_width=%d sdl_char_height=%d\n", 
         DEFAULT_FONTSZ, sdl_char_width, sdl_char_height);
    if (sdl_char_width != 50 || sdl_char_height != 83) {
        ERROR("chw,chh, expected = 50,83  actual = %d,%d\n", sdl_char_width, sdl_char_height);
        return -1; //xxx should this be an error ret
    }

    // init sensor code
    sdl_sensor_init_private();

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

    for (i = MIN_FONT_PTSIZE; i < MAX_FONT_PTSIZE; i++) {
        if (font[i] != NULL) {
            TTF_CloseFont(font[i]);
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
    evid_swipe_right_registered = false;
    evid_swipe_left_registered = false;
    evid_motion_registered = false;
    evid_keybd_registered = false;

    set_render_draw_color(color);
    SDL_RenderClear(renderer);
}

void sdl_display_present(void)
{
    SDL_RenderPresent(renderer);
}

// -----------------  EVENTS  -----------------------------

void sdl_register_event(sdl_loc_t *loc, int event_id)
{
    sdl_loc_t loc2;

    if (event_id == EVID_SWIPE_RIGHT) {
        evid_swipe_right_registered = true;
        return;
    }
    if (event_id == EVID_SWIPE_LEFT) {
        evid_swipe_left_registered = true;
        return;
    }
    if (event_id == EVID_MOTION) {
        evid_motion_registered = true;
        return;
    }
    if (event_id == EVID_KEYBD) {
        evid_keybd_registered = true;
        return;
    }

    if (loc == NULL || loc->w == 0 || loc->h == 0) {
        ERROR("invalid loc, event_id=%d\n", event_id);
        return;
    }

    // enforce minimum w,h
    loc2 = *loc;
    if (loc2.w < 150) {
        int delta = 150 - loc2.w;
        loc2.w += delta;
        loc2.x -= delta/2;
    }
    if (loc2.h < 150) {
        int delta = 150 - loc2.h;
        loc2.h += delta;
        loc2.y -= delta/2;
    }

    event_tbl[max_event].loc = loc2;
    event_tbl[max_event].event_id  = event_id; 
    max_event++;
}

//xxx check this
void sdl_register_control_events(char *ev1, char *ev2, char *ev3, int bg_color)
{
    sdl_loc_t *loc;
    int i, x, y;
    char *ev[3];
    sdl_print_state_t print_state;

    ev[0] = ev1;
    ev[1] = ev2;
    ev[2] = ev3;

    sdl_print_save(&print_state);

#define LARGE_FONTSZ    10

    sdl_print_init(LARGE_FONTSZ, COLOR_WHITE, bg_color);

    for (i = 0; i < 3; i++) {
        if (ev[i] == NULL) {
            continue;
        }

        x = (sdl_win_width/3/2) + i * (sdl_win_width/3);
        y = sdl_win_height - sdl_char_height/2;
        loc = sdl_render_text_xyctr(x, y, ev[i]);
        sdl_register_event(loc, EVID_CONTROL_EVENT_1+i);
    }

    sdl_print_restore(&print_state);
}

// arg timeout_us:
//   -1:     wait forever
//    0:     don't wait
//    usecs: timeout
void sdl_get_event(long timeout_us, sdl_event_t *event)
{
    SDL_Event ev;
    long waited = 0;
    bool got_event;

    // xxx move
    memset(event, 0, sizeof(*event));
    event->event_id = -1;

try_again:
    // get event
    got_event = SDL_PollEvent(&ev);

    // no event available, either return error or try again to get event
    if (!got_event) {
        if (timeout_us == 0) {
            // dont wait
            return;
        } else if (timeout_us < 0 || waited < timeout_us) {
            // either wait forever or time waited is less than timeout_us
            usleep(ONE_MS);
            waited += ONE_MS;
            goto try_again;
        } else {
            // time waited exceeds timeout_us
            return;
        }
    }

    // process the sdl_event; this may or may not return an event
    process_sdl_event(&ev, event);
    if (event->event_id == -1) {
        goto try_again;
    }

    // an event was returned from process_sdl_event
    return;
}

static void process_sdl_event(SDL_Event *ev, sdl_event_t *event)
{
    #define AT_LOC(X,Y,loc) (((X) >= (loc).x)            && \
                             ((X) <  (loc).x + (loc).w)  && \
                             ((Y) >= (loc).y)            && \
                             ((Y) <  (loc).y + (loc).h))

    int i;

    switch (ev->type) {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
        static int last_pressed_x = -1;
        static int last_pressed_y = -1;
        int x, y;
#if 0
       INFO("MOUSE_BUTTON button=%s state=%s x=%d y=%d\n",
               (ev->button.button == SDL_BUTTON_LEFT   ? "LEFT" :
                ev->button.button == SDL_BUTTON_MIDDLE ? "MIDDLE" :
                ev->button.button == SDL_BUTTON_RIGHT  ? "RIGHT" : "???"),
               (ev->button.down ? "DOWN" : "UP"),
               ev->button.x,
               ev->button.y);
#endif
        x = ev->button.x / scale;
        y = ev->button.y / scale;

        if (ev->button.down) {
            last_pressed_x = x;
            last_pressed_y = y;
        } else {
            int delta_x = x - last_pressed_x;
            int delta_y = y - last_pressed_y;

            INFO("button released xy = %d %d, delta xy = %d %d\n", x, y, delta_x, delta_y);

            if (delta_x > 500 && evid_swipe_right_registered) {
                INFO("got EVID_SWIPE_RIGHT %d %d\n", delta_x, delta_y);
                event->event_id = EVID_SWIPE_RIGHT;
                break;
            } else if (delta_x < -500 && evid_swipe_left_registered) {
                INFO("got EVID_SWIPE_LEFT %d %d\n", delta_x, delta_y);
                event->event_id = EVID_SWIPE_LEFT;
                break;
            }

            for (i = 0; i < max_event; i++) {
                if (AT_LOC(x, y, event_tbl[i].loc)) {
                    break;
                }
            }
            if (i < max_event &&
                AT_LOC(last_pressed_x, last_pressed_y, event_tbl[i].loc))
            {
                event->event_id = event_tbl[i].event_id;
            }
        }
        break; }
    case SDL_EVENT_MOUSE_MOTION: {
        if ((ev->motion.state & SDL_BUTTON_LMASK) && evid_motion_registered) {
            INFO("MOUSE_MOTION x=%f y=%f xrel=%f yrel=%f\n",
                ev->motion.x,
                ev->motion.y,
                ev->motion.xrel,
                ev->motion.yrel);

            event->event_id = EVID_MOTION;
            event->u.motion.x = ev->motion.x;
            event->u.motion.y = ev->motion.y;
            event->u.motion.xrel = ev->motion.xrel;
            event->u.motion.yrel = ev->motion.yrel;
        }
        break; }
    case SDL_EVENT_SENSOR_UPDATE: {
#if 1  // xxx why is step counter not working
        SDL_SensorEvent *x = &ev->sensor;
        if (x->which == 14 || x->which == 15) 
            INFO("SENSOR: which=%d data=%f %f %f %f %f %f timestamp=%ld\n",
                 x->which,
                 x->data[0], x->data[1], x->data[2], x->data[3], x->data[4], x->data[5],
                 x->sensor_timestamp);
        //sdl_sensor_event(x);
#endif
        break; }
#if 0
    case SDL_EVENT_TEXT_INPUT: {
        SDL_TextInputEvent *x = &ev->text;
        INFO("SDL_EVENT_TEXT_INPUT: '%s'\n", x->text);
        break; }
    case SDL_EVENT_TEXT_EDITING: {
        SDL_TextEditingEvent *x = &ev->edit;
        INFO("SDL_EVENT_TEXT_EDITING: '%s' %d %d\n", x->text, x->start, x->length);
        break; }
#endif
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
        SDL_KeyboardEvent *x = &ev->key;
        bool shift = (x->mod & SDL_KMOD_SHIFT) != 0;
        SDL_Keycode keycode;

        if (!evid_keybd_registered || x->down) {
            break;
        }

        keycode = SDL_GetKeyFromScancode(x->scancode, x->mod, false);  // xxx not always working
        INFO("GOT keycode 0x%x  shift=%d\n", keycode, shift);
        event->event_id = EVID_KEYBD;
        event->u.keybd.ch = keycode;
        break; }
    case SDL_EVENT_FINGER_DOWN:
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_MOTION: {
        // not used
        break; }
    case SDL_EVENT_QUIT: {
        event->event_id = EVID_QUIT;
        break; }

    default: {
        //INFO("event_type %d - not supported\n", ev->type);
        break; }
    }
}

char *sdl_get_input_str(char *prompt, bool numeric_keybd, int bg_color)
{
    static char input[100]; // xxx bounds check
    int         max_input;
    sdl_loc_t  *loc;
    sdl_event_t event;

    // init
    memset(input, 0, sizeof(input));
    max_input = 0;

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetNumberProperty(
            props, 
            SDL_PROP_TEXTINPUT_TYPE_NUMBER, 
            numeric_keybd ?  SDL_TEXTINPUT_TYPE_NUMBER : SDL_TEXTINPUT_TYPE_TEXT);
    SDL_StartTextInputWithProperties(window, props);

//xxx yyy how to restore
    sdl_print_init(DEFAULT_FONTSZ, COLOR_WHITE, bg_color);

    // 
    while (true) {
        // xxx
        sdl_display_init(bg_color);
        sdl_register_event(NULL, EVID_KEYBD);

        // display prompt
        sdl_render_printf(0, 200, "%s", prompt);

        // display input line
        loc = sdl_render_printf(0, 350, "%s", input);

        // display cursor
        sdl_render_printf(loc->x+loc->w, loc->y, "%s", "_");

        // xxx
        sdl_display_present();

        // wait for event
        sdl_get_event(-1, &event);

        // process event
        if (event.event_id == EVID_KEYBD) {
            int ch = event.u.keybd.ch;

            if (ch >= 0x20 && ch < 0x7f) {
                if (max_input < sizeof(input)) {
                    input[max_input++] = ch;
                }
                continue; 
            } else if (ch == '\b') {
                if (max_input > 0) {
                    input[--max_input] = '\0';
                }
            } if (ch == '\r') {
                break;
            }
        }

        if (event.event_id == EVID_QUIT) {
            input[0] = '\0';
            break;
        }
    }

    SDL_StopTextInput(window);
    sdl_print_init(DEFAULT_FONTSZ, COLOR_WHITE, COLOR_PURPLE);  // xxx restore
    SDL_DestroyProperties(props);

    return input;
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

// ported from http://www.noah.org/wiki/Wavelength_to_RGB_in_Python
// xxx violet not working well
int sdl_wavelength_to_color(int wavelength_arg)
{
    double wavelength = wavelength_arg;
    double attenuation;
    double gamma = 0.8;
    double R,G,B;

    if (wavelength >= 380 && wavelength <= 440) {
        double attenuation = 0.3 + 0.7 * (wavelength - 380) / (440 - 380);
        R = pow((-(wavelength - 440) / (440 - 380)) * attenuation, gamma);
        G = 0.0;
        B = pow(1.0 * attenuation, gamma);
    } else if (wavelength >= 440 && wavelength <= 490) {
        R = 0.0;
        G = pow((wavelength - 440) / (490 - 440), gamma);
        B = 1.0;
    } else if (wavelength >= 490 && wavelength <= 510) {
        R = 0.0;
        G = 1.0;
        B = pow(-(wavelength - 510) / (510 - 490), gamma);
    } else if (wavelength >= 510 && wavelength <= 580) {
        R = pow((wavelength - 510) / (580 - 510), gamma);
        G = 1.0;
        B = 0.0;
    } else if (wavelength >= 580 && wavelength <= 645) {
        R = 1.0;
        G = pow(-(wavelength - 645) / (645 - 580), gamma);
        B = 0.0;
    } else if (wavelength >= 645 && wavelength <= 750) {
        attenuation = 0.3 + 0.7 * (750 - wavelength) / (750 - 645);
        R = pow(1.0 * attenuation, gamma);
        G = 0.0;
        B = 0.0;
    } else {
        R = 0.0;
        G = 0.0;
        B = 0.0;
    }

    if (R < 0) R = 0; else if (R > 1) R = 1;
    if (G < 0) G = 0; else if (G > 1) G = 1;
    if (B < 0) B = 0; else if (B > 1) B = 1;

    return sdl_create_color(R*255, G*255, B*255, 255);
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

static struct {
    int ptsize;
    SDL_Color fg_color;
    SDL_Color bg_color;
} text;

void sdl_print_save(sdl_print_state_t *save)
{
    //*save = text;
    memcpy(save, &text, sizeof(text));
}

void sdl_print_restore(sdl_print_state_t *restore)
{
    //text = *restore;
    memcpy(&text, restore, sizeof(text));
}

void sdl_print_init_color(int fg_color, int bg_color)
{
    // save new font color
    memcpy(&text.fg_color, &fg_color, 4);
    memcpy(&text.bg_color, &bg_color, 4);
}

void sdl_print_init(double numchars, int fg_color, int bg_color)
{
    int ptsize;
    double chw_fp, chh_fp;

    // if numchars is -1 then the font size is not being changed 
    if (numchars == -1) { // xxx del
        goto change_font_color;
    }

    // determine real font ptsize to use;
    // note: rint() not used here so ptsize will round down
    chw_fp = (sdl_win_width / numchars) * scale;
    chh_fp = chw_fp / 0.6;
    ptsize = chh_fp;

    // ensure ptiszie is in range
    if (ptsize < MIN_FONT_PTSIZE) ptsize = MIN_FONT_PTSIZE;
    if (ptsize >= MAX_FONT_PTSIZE) ptsize = MAX_FONT_PTSIZE-1;

    // if the requested font pointsize has not yet been opened
    // then do so
    if (font[ptsize] == NULL) {
        font[ptsize] = TTF_OpenFont(FONT_FILE_PATH, ptsize);
        if (font[ptsize] == NULL) {
            ERROR("TTF_OpenFont failed, ptsize=%d\n", ptsize);
            return;
        }
    }

    // save new point size and character width/height
    text.ptsize = ptsize;
    sdl_char_width  = rint(sdl_win_width / numchars);  // xxx nearbyint
    sdl_char_height = rint(sdl_char_width / 0.6);

change_font_color:
    // save new font color
    memcpy(&text.fg_color, &fg_color, 4);
    memcpy(&text.bg_color, &bg_color, 4);
}

static sdl_loc_t *render_text(bool xy_is_ctr, int x, int y, char * str)
{
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_FRect     pos;
    static sdl_loc_t loc;

    //printf("xy_is_ctr = %d x=%d y=%d str='%s'\n", xy_is_ctr, x, y, str);

    // if font not initialized then return error
    if (font[text.ptsize] == NULL) {
        ERROR("font ptsize %d, not initialized\n", text.ptsize);
        loc.x = x; loc.y = y; loc.w = 0; loc.h = 0;
        return &loc;
    }

    // if zero len str then return
    if (str[0] == '\0') {
        loc.x = x; loc.y = y; loc.w = 0; loc.h = 0;
        return &loc;
    }

    // render the string to a surface
    surface = TTF_RenderText_Shaded(font[text.ptsize], str, 0, text.fg_color, text.bg_color);
    if (surface == NULL) {
        ERROR("TTF_RenderText_Shaded returned NULL\n");
        loc.x = x; loc.y = y; loc.w = 0; loc.h = 0;
        return &loc;
    }

    // determine the real display position to render the text
    // xxx dont need rint now
    if (!xy_is_ctr) {
        pos.x = x*scale;
        pos.y = y*scale;
        pos.w = surface->w;
        pos.h = surface->h;
    } else {
        pos.x = x*scale - surface->w/2.;
        pos.y = y*scale - surface->h/2.;
        pos.w = surface->w;
        pos.h = surface->h;
    }

    // create texture from the surface, and render the texture
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_RenderTexture(renderer, texture, NULL, &pos);

    // clean up
    SDL_DestroySurface(surface);
    SDL_DestroyTexture(texture);

    // return the display location where the text was rendered;
    loc.x = pos.x / scale;
    loc.y = pos.y / scale;
    loc.w = pos.w / scale;
    loc.h = pos.h / scale;

    return &loc;
}

// xxx explain str
void sdl_render_multiline_text(int y_top, int y_display_begin, int y_display_end, char * str)
{
    char line[1000], *p;
    int len;
    int y = y_top;

    while (str[0]) {
        // get line
        p = strchr(str, '\n');
        len = (p == NULL ? strlen(str) : p-str);
        memcpy(line, str, len);
        line[len] = '\0';
        //printf("LINE '%s'\n", line);

        // if y pos of line is below the bottom of the
        // display region then break
        if (y > y_display_end - sdl_char_height) {
            break;
        }

        // if y loc of line is at or below the begining of the display
        // region then render the line
        if (y >= y_display_begin && len > 0) {
            render_text(false, 0, y, line);
        }

        // advance y and str to the next line
        y += sdl_char_height;
        str += len;
        if (str[0] == '\n') str++;
    }
}

void sdl_render_multiline_text_2(int y_top, int y_display_begin, int y_display_end, char **lines, int n)
{
    int i;
    int y = y_top;

    for (i = 0; i < n; i++) {
        // if y pos of line is below the bottom of the
        // display region then break
        if (y > y_display_end - sdl_char_height) {
            break;
        }

        // if y loc of line is at or below the begining of the display
        // region then render the line
        if (y >= y_display_begin && lines[i][0] != '\0') {
            render_text(false, 0, y, lines[i]);
        }

        // advance y for the next line
        y += sdl_char_height;
    }
}

sdl_loc_t *sdl_render_text(int x, int y, char * str)
{
    return render_text(false, x, y, str);
}

sdl_loc_t *sdl_render_text_xyctr(int x, int y, char * str)
{
    return render_text(true, x, y, str);
}

sdl_loc_t *sdl_render_printf(int x, int y, char * fmt, ...)
{
    char str[1000];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(str, sizeof(str), fmt, ap);
    va_end(ap);

    return sdl_render_text(x, y, str);
}

sdl_loc_t *sdl_render_printf_xyctr(int x, int y, char * fmt, ...)
{
    char str[1000];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(str, sizeof(str), fmt, ap);
    va_end(ap);

    return sdl_render_text_xyctr(x, y, str);
}

// -----------------  RENDER RECTANGLES, LINES, CIRCLES, POINTS  --------------------

void sdl_render_rect(int x, int y, int w, int h, int line_width, int color)
{
    SDL_FRect rect;
    int i;

    rect.x = x * scale;
    rect.y = y * scale;
    rect.w = w * scale;
    rect.h = h * scale;

    set_render_draw_color(color);

    for (i = 0; i < line_width; i++) {
        SDL_RenderRect(renderer, &rect);
        if (rect.w < 2 || rect.h < 2) {
            break;
        }
        rect.x += 1;
        rect.y += 1;
        rect.w -= 2;
        rect.h -= 2;
    }
}

void sdl_render_fill_rect(int x, int y, int w, int h, int color)
{
    SDL_FRect rect;

    rect.x = x * scale;
    rect.y = y * scale;
    rect.w = w * scale;
    rect.h = h * scale;

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
    SDL_FPoint scaled_points[100];  // xxx malloc this

    if (count <= 1) {
        return;
    }

    for (int i = 0; i < count; i++) {
        scaled_points[i].x = points[i].x * scale;
        scaled_points[i].y = points[i].y * scale;
    }

    set_render_draw_color(color);

    SDL_RenderLines(renderer, scaled_points, count);
}

// xxx change args to x_ctr_arg ..
void sdl_render_circle(int x_ctr_arg, int y_ctr_arg, int radius, int line_width, int color)
{
    int count = 0, i, angle, x, y;
    int x_center, y_center;
    SDL_FPoint points[370];

    static int sin_table[370];
    static int cos_table[370];
    static bool first_call = true;

    // xxx comment
    x_center = rint(x_ctr_arg * scale);
    y_center = rint(y_ctr_arg * scale);
    radius   = rint(radius * scale);

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
        SDL_RenderLines(renderer, points, count);
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
    SDL_FPoint sdl_points[MAX_SDL_POINTS];
    int sdl_points_count = 0;
    struct point_extend_s * pe = &point_extend[point_size];
    struct point_extend_offset_s * peo = pe->offset;

    if (count < 0) {
        return;
    }
    if (point_size < 0) {
        point_size = 0;
    }
    if (point_size > 9) {
        point_size = 9;
    }

    set_render_draw_color(color);

    for (i = 0; i < count; i++) {
        for (j = 0; j < pe->max; j++) {
            x = rint((points[i].x + peo[j].x) * scale);
            y = rint((points[i].y + peo[j].y) * scale);
            sdl_points[sdl_points_count].x = x;
            sdl_points[sdl_points_count].y = y;
            sdl_points_count++;

            if (sdl_points_count == MAX_SDL_POINTS) {
                SDL_RenderPoints(renderer, sdl_points, sdl_points_count);
                sdl_points_count = 0;
            }
        }
    }

    if (sdl_points_count > 0) {
        SDL_RenderPoints(renderer, sdl_points, sdl_points_count);
        sdl_points_count = 0;
    }
}

// -----------------  RENDER USING TEXTURES  ---------------------------- 

sdl_texture_t *sdl_create_texture_from_pixels(sdl_pixels_t *pixels)
{
    sdl_texture_t *texture;

    // create the texture
    texture = (sdl_texture_t*)
              SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ABGR8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                pixels->w, pixels->h);
    if (texture == NULL) {
        ERROR("failed to allocate texture\n");
        return NULL;
    }

    // update the texture with the pixels
    SDL_UpdateTexture((SDL_Texture*)texture, NULL, pixels->pixels, pixels->w * BYTES_PER_PIXEL);

    // return the texture
    return texture;
}

sdl_texture_t *sdl_create_filled_circle_texture(int radius, int color)
{
    radius *= scale;

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
    texture = (sdl_texture_t*)
              SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ABGR8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                width, width);
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
    if (font[text.ptsize] == NULL) {
        ERROR("font ptsize %d, not initialized\n", text.ptsize);
        return NULL;
    }

    // render the text to a surface,
    // create a texture from the surface
    // free the surface
    surface = TTF_RenderText_Shaded(font[text.ptsize], str, 0, text.fg_color, text.bg_color);
    if (surface == NULL) {
        ERROR("failed to allocate surface\n");
        return NULL;
    }
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == NULL) {
        ERROR("failed to allocate texture\n");
        SDL_DestroySurface(surface);
        return NULL;
    }
    SDL_DestroySurface(surface);

    // return the texture which contains the text
    return (sdl_texture_t*)texture;
}

void sdl_render_texture(int x, int y, int w, int h, double angle, sdl_texture_t *texture)
{
    SDL_FRect dest;
    float w_float, h_float;

    if (texture == NULL) {
        return;
    }

    if (w == -1 || h == -1) {  // xxx check these both yield the same result
        SDL_GetTextureSize((SDL_Texture *)texture, &w_float, &h_float);
        dest.x = x * scale;
        dest.y = y * scale;
        dest.w = w_float;
        dest.h = h_float;
    } else {
        dest.x = x * scale;
        dest.y = y * scale;
        dest.w = w * scale;
        dest.h = h * scale;
    }

    SDL_RenderTextureRotated(renderer, (SDL_Texture*)texture, NULL, &dest, angle, NULL, false);
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
    float w_float, h_float;

    if (texture == NULL) {
        *width = 0;
        *height = 0;
        return;
    }

    SDL_GetTextureSize((SDL_Texture *)texture, &w_float, &h_float);
    *width = rint(w_float / scale);
    *height = rint(h_float / scale);
}

// caller must free pixels
sdl_pixels_t *sdl_read_display_pixels(int x, int y, int w, int h)
{
    sdl_pixels_t *pixels;
    SDL_Rect      loc;
    int           malloc_len;
    SDL_Surface  *surface;

    loc.x = x * scale; // xxx rint
    loc.y = y * scale;
    loc.w = w * scale;
    loc.h = h * scale;

    // read the pixels  xxx check all rets
    surface = SDL_RenderReadPixels(renderer, &loc);
    if (surface == NULL) {
        ERROR("SDL_RenderReadPixels, %s\n", SDL_GetError());
        return NULL;
    }

    // allocate memory for the pixels
    malloc_len = sizeof(sdl_pixels_t) + loc.w * loc.h * BYTES_PER_PIXEL;
    pixels = malloc(malloc_len);
    if (pixels == NULL) {
        ERROR("allocate pixels failed\n");
        SDL_DestroySurface(surface);
        return NULL;
    }

    // init return pixels struct
    pixels->magic      = PIXELS_MAGIC;
    pixels->struct_len = malloc_len;
    pixels->w          = loc.w;
    pixels->h          = loc.h;
    void *pxls = pixels->pixels;
    for (int row = 0; row < loc.h; row++) {
        memcpy(pxls, 
               surface->pixels + (row * surface->pitch), 
               loc.w * BYTES_PER_PIXEL);
        pxls += (loc.w * BYTES_PER_PIXEL);
    }

    // destroy surface
    SDL_DestroySurface(surface);

    // success, return allocated sdl_pixels_t   
    return pixels;
}

