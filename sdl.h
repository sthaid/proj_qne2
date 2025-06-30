#include <SDL.h>
#include <SDL_ttf.h>

//                          R         G        B         A
#define COLOR_PURPLE     ( 127  |    0<<8 |  255<<16 |  255<<24 )
#define COLOR_BLUE       ( 0    |    0<<8 |  255<<16 |  255<<24 )
#define COLOR_LIGHT_BLUE ( 0    |  255<<8 |  255<<16 |  255<<24 )
#define COLOR_GREEN      ( 0    |  255<<8 |    0<<16 |  255<<24 )
#define COLOR_YELLOW     ( 255  |  255<<8 |    0<<16 |  255<<24 )
#define COLOR_ORANGE     ( 255  |  128<<8 |    0<<16 |  255<<24 )
#define COLOR_PINK       ( 255  |  105<<8 |  180<<16 |  255<<24 )
#define COLOR_RED        ( 255  |    0<<8 |    0<<16 |  255<<24 )
#define COLOR_GRAY       ( 224  |  224<<8 |  224<<16 |  255<<24 )
#define COLOR_WHITE      ( 255  |  255<<8 |  255<<16 |  255<<24 )
#define COLOR_BLACK      ( 0    |    0<<8 |    0<<16 |  255<<24 )

#define MAX_FONT_PTSIZE       200  // xxx check for out of range

int32_t sdl_init(int *w, int *h);
void sdl_exit(void);

void sdl_display_init(uint32_t color);
void sdl_display_present(void);

void sdl_render_text(int32_t x, int32_t y, char * str);
void sdl_render_printf(int32_t x, int32_t y, char * fmt, ...) __attribute__ ((format (printf, 3, 4)));
void sdl_set_text_ptsize(int32_t ptsize);
void sdl_set_text_fg_color(uint32_t color);
void sdl_set_text_bg_color(uint32_t color);

uint32_t sdl_create_color(int r, int g, int b, int a);
uint32_t sdl_scale_color(uint32_t color, double inten);
void sdl_set_render_draw_color(uint32_t color);

