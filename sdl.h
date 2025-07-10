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

#define MAX_FONT_PTSIZE       200  // xxx check for out of range

//
// typedefs
//

typedef struct {
    short x, y, w, h;
} sdl_rect_t;

typedef struct {
    int x, y;
} sdl_point_t;

typedef void * sdl_texture_t;


//
// prototypes
//

int sdl_init(int *w, int *h);
void sdl_exit(void);

void sdl_display_init(int color);
void sdl_display_present(void);

// xxx order and comments

sdl_rect_t *sdl_render_text(int x, int y, int ptsize, int fg_color, int bg_color, char *str);
sdl_rect_t *sdl_render_printf(int x, int y, int ptsize, int fg_color, int bg_color, char *fmt, ...) 
        __attribute__ ((format (printf, 6, 7)));
void sdl_get_char_size(int ptsize, int *char_width, int *char_height);

int sdl_create_color(int r, int g, int b, int a);
int sdl_scale_color(int color, double inten);

void sdl_register_event(sdl_rect_t *loc, int event_id);
int sdl_get_event(long timeout_us);

// render rectangle, lines, circles, points
void sdl_render_rect(sdl_rect_t *loc, int line_width, int color);
void sdl_render_fill_rect(sdl_rect_t *loc, int color);
void sdl_render_line(int x1, int y1, int x2, int y2, int color);
void sdl_render_lines(sdl_point_t *points, int count, int color);
void sdl_render_circle(int x_center, int y_center, int radius, int line_width, int color);
void sdl_render_point(int x, int y, int color, int point_size);
void sdl_render_points(sdl_point_t *points, int count, int color, int point_size);

// render using textures
sdl_texture_t sdl_create_texture(int w, int h);
sdl_texture_t sdl_create_texture_from_pane_pixels(sdl_rect_t *pane);
sdl_texture_t sdl_create_filled_circle_texture(int radius, int color);
sdl_texture_t sdl_create_text_texture(int ptsize, int fg_color, int bg_color, char *str);
void sdl_destroy_texture(sdl_texture_t texture);

void sdl_query_texture(sdl_texture_t texture, int *width, int *height);
void sdl_update_texture(sdl_texture_t texture, uint8_t *pixels, int pitch);
void sdl_render_texture(int x, int y, sdl_texture_t texture);
void sdl_render_scaled_texture(sdl_rect_t *dest, sdl_texture_t texture);
