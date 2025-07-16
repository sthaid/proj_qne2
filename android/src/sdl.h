#include <SDL.h>
#include <SDL_ttf.h>

extern const int COLOR_PURPLE;
extern const int COLOR_BLUE;
extern const int COLOR_LIGHT_BLUE;
extern const int COLOR_GREEN;
extern const int COLOR_YELLOW;
extern const int COLOR_ORANGE;
extern const int COLOR_PINK;
extern const int COLOR_RED;
extern const int COLOR_GRAY;
extern const int COLOR_WHITE;
extern const int COLOR_BLACK;

// xxx how to get defines in picoc
#define EVID_SWIPE_DOWN        1000
#define EVID_SWIPE_UP          1001
#define EVID_SWIPE_RIGHT       1002
#define EVID_SWIPE_LEFT        1003
#define EVID_QUIT              9999

//
// typedefs
//

typedef struct {
    int x, y, w, h;
} sdl_rect_t;

typedef struct {
    int x, y;
} sdl_point_t;

typedef struct sdl_texture sdl_texture_t;

//
// prototypes
//

// sdl initialization and termination, must be done once
int sdl_init(void);
void sdl_exit(void);
void sdl_get_win_size(int *w, int *h);

// display init and present, must be done for every display update
void sdl_display_init(int color);
void sdl_display_present(void);

// event registration and query
void sdl_register_event(sdl_rect_t *loc, int event_id);
int sdl_get_event(long timeout_us);

// create colors
int sdl_create_color(int r, int g, int b, int a);
int sdl_scale_color(int color, double inten);
int sdl_wavelength_to_color(int wavelength);

// render text
void sdl_print_init(double numchars, int fg_color, int bg_color, int *char_width, int *char_height, int *win_rows, int *win_cols);
sdl_rect_t *sdl_render_text(bool xy_is_ctr, int x, int y, char *str);
sdl_rect_t *sdl_render_printf(bool xy_is_ctr, int x, int y, char *fmt, ...) __attribute__ ((format (printf, 4, 5)));

// render rectangle, lines, circles, points
void sdl_render_rect(bool xy_is_ctr, int x, int y, int w, int h, int line_width, int color);
void sdl_render_fill_rect(bool xy_is_ctr, int x, int y, int w, int h, int color);
void sdl_render_line(int x1, int y1, int x2, int y2, int color);
void sdl_render_lines(sdl_point_t *points, int count, int color);
void sdl_render_circle(bool xy_is_ctr, int x, int y, int radius, int line_width, int color);
void sdl_render_point(int x, int y, int color, int point_size);
void sdl_render_points(sdl_point_t *points, int count, int color, int point_size);

// render using textures
sdl_texture_t *sdl_create_texture(int w, int h);
sdl_texture_t *sdl_create_texture_from_display(bool xy_is_ctr, int x, int y, int w, int h);
sdl_texture_t *sdl_create_filled_circle_texture(int radius, int color);
sdl_texture_t *sdl_create_text_texture(char *str);
void sdl_destroy_texture(sdl_texture_t *texture);
void sdl_query_texture(sdl_texture_t *texture, int *w, int *h);
void sdl_update_texture(sdl_texture_t *texture, char *pixels, int pitch);
void sdl_render_texture(bool xy_is_ctr, int x, int y, sdl_texture_t *texture);
void sdl_render_scaled_texture(bool xy_is_ctr, int x, int y, int w, int h, sdl_texture_t *texture);
void sdl_render_rotated_texture(bool xy_is_ctr, int x, int y, double angle, sdl_texture_t *texture);

