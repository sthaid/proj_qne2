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

struct sdl_rect {
    short x;
    short y;
    short w;
    short h;
};

int sdl_init(int *w, int *h);   // okay
void sdl_exit(void);            // okay

void sdl_display_init(int color);  // okay
void sdl_display_present(void);    // okay

struct sdl_rect *sdl_render_text(int x, int y, char *str);  // okay, xxx but are both needed
struct sdl_rect *sdl_render_printf(int x, int y, char *fmt, ...) __attribute__ ((format (printf, 3, 4)));   // okay
void sdl_set_text_ptsize(int ptsize);  // okay
void sdl_set_text_fg_color(int color);  // okay
void sdl_set_text_bg_color(int color);  // okay
void sdl_get_char_size(int *char_width, int *char_height);  // okay

int sdl_create_color(int r, int g, int b, int a);  // okay
int sdl_scale_color(int color, double inten);  // okay
void sdl_set_render_draw_color(int color);    // okay

void sdl_register_event(struct sdl_rect *loc, int event_id);  // okay
int sdl_get_event(long timeout_us);  // okay

// --------- xxx ------ 

// render rectangle, lines, circles, points
void sdl_render_rect(struct sdl_rect *loc, int32_t line_width, int32_t color);
void sdl_render_fill_rect(struct sdl_rect *loc, int32_t color);
void sdl_render_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t color);
void sdl_render_lines(point_t *points, int32_t count, int32_t color);
void sdl_render_circle(int32_t x_center, int32_t y_center, int32_t radius,
            int32_t line_width, int32_t color);
void sdl_render_point(int32_t x, int32_t y, int32_t color, int32_t point_size);
void sdl_render_points(point_t *points, int32_t count, int32_t color, int32_t point_size);

// render using textures
texture_t sdl_create_texture(int32_t w, int32_t h);
texture_t sdl_create_texture_from_pane_pixels(struct sdl_rect *pane);
texture_t sdl_create_filled_circle_texture(int32_t radius, int32_t color);
texture_t sdl_create_text_texture(int32_t fg_color, int32_t bg_color, int32_t font_ptsize, char *str);
void sdl_destroy_texture(texture_t texture);

void sdl_query_texture(texture_t texture, int32_t *width, int32_t *height);
void sdl_update_texture(texture_t texture, uint8_t *pixels, int32_t pitch);
struct sdl_rect sdl_render_texture(int32_t x, int32_t y, texture_t texture);

//struct sdl_rect sdl_render_scaled_texture(struct sdl_rect *loc, texture_t texture);
//void sdl_render_scaled_texture_ex(struct sdl_rect *pane, struct sdl_rect *src, struct sdl_rect *dst, texture_t texture);


