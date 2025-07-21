#include <SDL.h>
#include <SDL_ttf.h>

// https://www.w3schools.com/colors/colors_converter.asp

#define BYTES_PER_PIXEL  4
#define COLOR_BLACK      (   0  |    0<<8 |    0<<16 |  255<<24 )
#define COLOR_WHITE      ( 255  |  255<<8 |  255<<16 |  255<<24 )
#define COLOR_RED        ( 255  |    0<<8 |    0<<16 |  255<<24 )
#define COLOR_ORANGE     ( 255  |  128<<8 |    0<<16 |  255<<24 )
#define COLOR_YELLOW     ( 255  |  255<<8 |    0<<16 |  255<<24 )
#define COLOR_GREEN      (   0  |  255<<8 |    0<<16 |  255<<24 )
#define COLOR_BLUE       (   0  |    0<<8 |  255<<16 |  255<<24 )
#define COLOR_INDIGO     (  75  |    0<<8 |  130<<16 |  255<<24 )
#define COLOR_VIOLET     ( 238  |  130<<8 |  238<<16 |  255<<24 )
#define COLOR_PURPLE     ( 127  |    0<<8 |  255<<16 |  255<<24 )
#define COLOR_LIGHT_BLUE (   0  |  255<<8 |  255<<16 |  255<<24 )
#define COLOR_PINK       ( 255  |  105<<8 |  180<<16 |  255<<24 )
#define COLOR_TEAL       (   0  |  128<<8 |  128<<16 |  255<<24 )
#define COLOR_LIGHT_GRAY ( 192  |  192<<8 |  192<<16 |  255<<24 )
#define COLOR_GRAY       ( 128  |  128<<8 |  128<<16 |  255<<24 )
#define COLOR_DARK_GRAY  (  64  |   64<<8 |   64<<16 |  255<<24 )

#define EVID_SWIPE_RIGHT       9990
#define EVID_SWIPE_LEFT        9991
#define EVID_MOTION            9992
#define EVID_QUIT              9999

//
// typedefs
//

typedef struct {
    int x, y, w, h;
} sdl_loc_t;

typedef struct {
    int x, y;
} sdl_point_t;

typedef struct sdl_texture sdl_texture_t;

typedef struct {
    int event_id;
    union {
        struct {
            int x, y, xrel, yrel;
        } motion;
    } u;
} sdl_event_t;

//
// global variables
//

extern int sdl_win_width;
extern int sdl_win_height;
extern int sdl_char_width;
extern int sdl_char_height;

//
// prototypes
//

// sdl initialization and termination, must be done once
int sdl_init(void);
void sdl_exit(void);

// display init and present, must be done for every display update
void sdl_display_init(int color);
void sdl_display_present(void);

// event registration and query
void sdl_register_event(sdl_loc_t *loc, int event_id);
void sdl_get_event(long timeout_us, sdl_event_t *event);

// create colors
int sdl_create_color(int r, int g, int b, int a);
int sdl_scale_color(int color, double inten);
int sdl_wavelength_to_color(int wavelength);

// render text
void sdl_print_init(double numchars, int fg_color, int bg_color);
sdl_loc_t *sdl_render_text(int x, int y, char *str);
sdl_loc_t *sdl_render_printf(int x, int y, char *fmt, ...) __attribute__ ((format (printf, 3, 4)));
sdl_loc_t *sdl_render_text_xyctr(int x, int y, char *str);
sdl_loc_t *sdl_render_printf_xyctr(int x, int y, char *fmt, ...) __attribute__ ((format (printf, 3, 4)));
void sdl_render_multiline_text(int y_top, int y_display_begin, int y_display_end, char * str);

// render rectangle, lines, circles, points
void sdl_render_rect(int x, int y, int w, int h, int line_width, int color);
void sdl_render_fill_rect(int x, int y, int w, int h, int color);
void sdl_render_line(int x1, int y1, int x2, int y2, int color);
void sdl_render_lines(sdl_point_t *points, int count, int color);
void sdl_render_circle(int x_ctr, int y_ctr, int radius, int line_width, int color);
void sdl_render_point(int x, int y, int color, int point_size);
void sdl_render_points(sdl_point_t *points, int count, int color, int point_size);

// render using textures
sdl_texture_t *sdl_create_texture_from_display(int x, int y, int w, int h);
sdl_texture_t *sdl_create_texture_from_pixels(int w, int h, int *pixels);
sdl_texture_t *sdl_create_filled_circle_texture(int radius, int color);
sdl_texture_t *sdl_create_text_texture(char *str);
void sdl_render_texture(int x, int y, int w, int h, double angle, sdl_texture_t *texture);
void sdl_destroy_texture(sdl_texture_t *texture);
void sdl_update_texture(sdl_texture_t *texture, int *pixels);  //xxx region
void sdl_query_texture(sdl_texture_t *texture, int *w, int *h);
