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

typedef struct {
    short x, y, w, h;
} sdl_rect_t;

// point
typedef struct {
    int x, y; //xxx short?
} sdl_point_t;

// texture
typedef void * sdl_texture_t;


int sdl_init(int *w, int *h);   // okay
void sdl_exit(void);            // okay

void sdl_display_init(int color);  // okay
void sdl_display_present(void);    // okay

sdl_rect_t *sdl_render_text(int x, int y, char *str);  // okay, xxx but are both needed
sdl_rect_t *sdl_render_printf(int x, int y, char *fmt, ...) __attribute__ ((format (printf, 3, 4)));   // okay
void sdl_set_text_ptsize(int ptsize);  // okay
void sdl_set_text_fg_color(int color);  // okay
void sdl_set_text_bg_color(int color);  // okay
void sdl_get_char_size(int *char_width, int *char_height);  // okay

int sdl_create_color(int r, int g, int b, int a);  // okay
int sdl_scale_color(int color, double inten);  // okay
void sdl_set_render_draw_color(int color);    // okay

void sdl_register_event(sdl_rect_t *loc, int event_id);  // okay
int sdl_get_event(long timeout_us);  // okay

// --------- xxx ------ 

// render rectangle, lines, circles, points
void sdl_render_rect(sdl_rect_t *loc, int line_width, int color);
void sdl_render_fill_rect(sdl_rect_t *loc, int color);
void sdl_render_line(int x1, int y1, int x2, int y2, int color);
void sdl_render_lines(sdl_point_t *points, int count, int color);
void sdl_render_circle(int x_center, int y_center, int radius,
            int line_width, int color);
void sdl_render_point(int x, int y, int color, int point_size);
void sdl_render_points(sdl_point_t *points, int count, int color, int point_size);

// render using textures
sdl_texture_t sdl_create_texture(int w, int h);
sdl_texture_t sdl_create_texture_from_pane_pixels(sdl_rect_t *pane);
sdl_texture_t sdl_create_filled_circle_texture(int radius, int color);
sdl_texture_t sdl_create_text_texture(int fg_color, int bg_color, int font_ptsize, char *str);
void sdl_destroy_texture(sdl_texture_t texture);

void sdl_query_texture(sdl_texture_t texture, int *width, int *height);
void sdl_update_texture(sdl_texture_t texture, uint8_t *pixels, int pitch);
void sdl_render_texture(int x, int y, sdl_texture_t texture);
void sdl_render_scaled_texture(sdl_rect_t *dest, sdl_texture_t texture);

// xxx
//void sdl_render_scaled_texture_ex(sdl_rect_t *pane, sdl_rect_t *src, sdl_rect_t *dst, sdl_texture_t texture);

