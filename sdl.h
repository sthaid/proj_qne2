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

int sdl_init(int *w, int *h);
void sdl_exit(void);

void sdl_display_init(int color);
void sdl_display_present(void);

struct sdl_rect *sdl_render_text(int x, int y, char * str);
struct sdl_rect *sdl_render_printf(int x, int y, char * fmt, ...) __attribute__ ((format (printf, 3, 4)));
void sdl_set_text_ptsize(int ptsize);
void sdl_set_text_fg_color(int color);
void sdl_set_text_bg_color(int color);
void sdl_get_char_size(int *char_width, int *char_height);

int sdl_create_color(int r, int g, int b, int a);
int sdl_scale_color(int color, double inten);
void sdl_set_render_draw_color(int color);

void sdl_register_event(struct sdl_rect *loc, int event_id);
int sdl_get_event(bool wait);

