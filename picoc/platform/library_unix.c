#include "../interpreter.h"
#include <sdl.h>

struct StdVararg
{
    struct Value **Param;
    int NumArgs;
};

int StdioBasePrintf(struct ParseState *Parser, FILE *Stream, char *StrOut,
    int StrOutLen, char *Format, struct StdVararg *Args);

// -----------------  SDL PLATFORM ROUTINES  ----------------------------

//
// sdl initialization and termination, must be done once
//

void Sdl_init (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int ret;

    ret = sdl_init();

    ReturnValue->Val->Integer = ret;
}

void Sdl_exit (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_exit();
}

void Sdl_get_win_size (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int *w = Param[0]->Val->Pointer;
    int *h = Param[1]->Val->Pointer;

    sdl_get_win_size(w, h);
}

//
// display init and present, must be done for every display update
//

void Sdl_display_init (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int color = Param[0]->Val->Integer;

    sdl_display_init(color);
}

void Sdl_display_present (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_display_present();
}

//
// event registration and query
//

void Sdl_register_event (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_rect_t *loc      = (sdl_rect_t*)Param[0]->Val->Pointer;
    int         event_id = Param[1]->Val->Integer;

    sdl_register_event(loc, event_id);
}

void Sdl_get_event (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    long timeout_us = Param[0]->Val->LongInteger;
    int  event_id;

    event_id = sdl_get_event(timeout_us);

    ReturnValue->Val->Integer = event_id;
}

//
// create colors
//

void Sdl_create_color (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int r = Param[0]->Val->Integer;
    int g = Param[1]->Val->Integer;
    int b = Param[2]->Val->Integer;
    int a = Param[3]->Val->Integer;
    int color;

    color = sdl_create_color(r, g, b, a);
    
    ReturnValue->Val->Integer = color;
}

void Sdl_scale_color (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int color = Param[0]->Val->Integer;
    double inten = Param[1]->Val->FP;
    int scaled_color;

    scaled_color = sdl_scale_color(color, inten);

    ReturnValue->Val->Integer = scaled_color;
}

void Sdl_wavelength_to_color (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int wavelength = Param[0]->Val->Integer;
    int color;

    color = sdl_wavelength_to_color(wavelength);

    ReturnValue->Val->Integer = color;
}

//
// render text
//

static int Char_width;

void Sdl_print_init (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    double numchars    = Param[0]->Val->FP;
    int    fg_color    = Param[1]->Val->Integer;
    int    bg_color    = Param[2]->Val->Integer;
    int   *char_width  = Param[3]->Val->Pointer;
    int   *char_height = Param[4]->Val->Pointer;
    int   *win_rows    = Param[5]->Val->Pointer;
    int   *win_cols    = Param[6]->Val->Pointer;
    int    tmp_char_width;

    if (char_width == NULL) {
        char_width = &tmp_char_width;
    }

    sdl_print_init(numchars, fg_color, bg_color, char_width, char_height, win_rows, win_cols);

    Char_width = *char_width;
}

void Sdl_render_text (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    bool        xy_is_ctr = Param[0]->Val->Integer;
    int         x         = Param[1]->Val->Integer;
    int         y         = Param[2]->Val->Integer;
    char       *str       = Param[3]->Val->Pointer;
    sdl_rect_t *loc;

    loc = sdl_render_text(xy_is_ctr, x, y, str);

    ReturnValue->Val->Pointer = loc;
}

// xxx add nk version of above too

void Sdl_render_printf (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    bool  xy_is_ctr = Param[0]->Val->Integer;
    int   x         = Param[1]->Val->Integer;
    int   y         = Param[2]->Val->Integer;
    char *fmt       = Param[3]->Val->Pointer;

    struct StdVararg PrintfArgs;
    char             str[200] = "";
    sdl_rect_t      *loc;

    PrintfArgs.Param = Param + 3;
    PrintfArgs.NumArgs = NumArgs - 4;
    StdioBasePrintf(Parser, NULL, str, sizeof(str), fmt, &PrintfArgs);

    loc = sdl_render_text(xy_is_ctr, x, y, str);

    ReturnValue->Val->Pointer = loc;
}

#if 0
void Sdl_render_printf_nk (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int              n        = Param[0]->Val->Integer;
    int              k        = Param[1]->Val->Integer;
    int              y        = Param[2]->Val->Integer;
    char            *fmt      = Param[3]->Val->Pointer;

    struct StdVararg PrintfArgs;
    char             str[200] = "";
    sdl_rect_t      *loc;
    int              x;

    #define WIN_WIDTH 1000 //xxx ?

    PrintfArgs.Param = Param + 3;
    PrintfArgs.NumArgs = NumArgs - 4;
    StdioBasePrintf(Parser, NULL, str, sizeof(str), fmt, &PrintfArgs);

    // xxx what if n or k are invalid
    x = ((WIN_WIDTH/2/(n)) + (k) * (WIN_WIDTH/(n)) - strlen(str) * Char_width / 2);
    loc = sdl_render_text(x, y, str);

    ReturnValue->Val->Pointer = loc;
}
#endif

//
// render rectangle, lines, circles, points
//

void Sdl_render_rect (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    bool  xy_is_ctr  = Param[0]->Val->Integer;
    int   x          = Param[1]->Val->Integer;
    int   y          = Param[2]->Val->Integer;
    int   w          = Param[3]->Val->Integer;
    int   h          = Param[4]->Val->Integer;
    int   line_width = Param[5]->Val->Integer;
    int   color      = Param[6]->Val->Integer;

    sdl_render_rect(xy_is_ctr, x, y, w, h, line_width, color);
}

void Sdl_render_fill_rect (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    bool  xy_is_ctr  = Param[0]->Val->Integer;
    int   x          = Param[1]->Val->Integer;
    int   y          = Param[2]->Val->Integer;
    int   w          = Param[3]->Val->Integer;
    int   h          = Param[4]->Val->Integer;
    int   color      = Param[5]->Val->Integer;

    sdl_render_fill_rect(xy_is_ctr, x, y, w, h, color);
}

void Sdl_render_line (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int x1    = Param[0]->Val->Integer;
    int y1    = Param[1]->Val->Integer;
    int x2    = Param[2]->Val->Integer;
    int y2    = Param[3]->Val->Integer;
    int color = Param[4]->Val->Integer;

    sdl_render_line(x1, y1, x2, y2, color);
}

void Sdl_render_lines (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_point_t *points = (sdl_point_t*)Param[0]->Val->Pointer;
    int count           = Param[1]->Val->Integer;
    int color           = Param[2]->Val->Integer;

    sdl_render_lines(points, count, color);
}

void Sdl_render_circle (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    bool xy_is_ctr  = Param[0]->Val->Integer;
    int  x          = Param[1]->Val->Integer;
    int  y          = Param[2]->Val->Integer;
    int  radius     = Param[3]->Val->Integer;
    int  line_width = Param[4]->Val->Integer;
    int  color      = Param[5]->Val->Integer;

    sdl_render_circle(xy_is_ctr, x, y, radius, line_width, color);
}

void Sdl_render_point (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int x          = Param[0]->Val->Integer;
    int y          = Param[1]->Val->Integer;
    int color      = Param[2]->Val->Integer;
    int point_size = Param[3]->Val->Integer;

    sdl_render_point(x, y, color, point_size);
}

void Sdl_render_points (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_point_t *points = (sdl_point_t*)Param[0]->Val->Pointer;
    int count           = Param[1]->Val->Integer;
    int color           = Param[2]->Val->Integer;
    int point_size      = Param[3]->Val->Integer;

    sdl_render_points(points, count, color, point_size);
}

//
// render using textures
//

void Sdl_create_texture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int w = Param[0]->Val->Integer;
    int h = Param[1]->Val->Integer;
    sdl_texture_t *texture;

    texture = sdl_create_texture(w, h);
    ReturnValue->Val->Pointer = (char*)texture; 
}

void Sdl_create_texture_from_display (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    bool  xy_is_ctr  = Param[0]->Val->Integer;
    int   x          = Param[1]->Val->Integer;
    int   y          = Param[2]->Val->Integer;
    int   w          = Param[3]->Val->Integer;
    int   h          = Param[4]->Val->Integer;

    sdl_texture_t *texture;

    texture = sdl_create_texture_from_display(xy_is_ctr, x, y, w, h);
    ReturnValue->Val->Pointer = (char*)texture; 
}

void Sdl_create_filled_circle_texture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int radius = Param[0]->Val->Integer;
    int color  = Param[1]->Val->Integer;
    sdl_texture_t *texture;

    texture = sdl_create_filled_circle_texture(radius, color);
    ReturnValue->Val->Pointer = (char*)texture; 
}

void Sdl_create_text_texture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *str = (char*)Param[0]->Val->Pointer;
    sdl_texture_t *texture;

    texture = sdl_create_text_texture(str);
    ReturnValue->Val->Pointer = (char*)texture; 
}

void Sdl_destroy_texture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_texture_t *texture = (sdl_texture_t*)Param[0]->Val->Pointer;

    sdl_destroy_texture(texture);
}

void Sdl_query_texture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_texture_t *texture = (sdl_texture_t*)Param[0]->Val->Pointer;
    int           *width   = (int*)Param[1]->Val->Pointer;
    int           *height  = (int*)Param[2]->Val->Pointer;

    sdl_query_texture(texture, width, height);
}

void Sdl_update_texture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_texture_t *texture = (sdl_texture_t*)Param[0]->Val->Pointer;
    char          *pixels  = (char*)Param[1]->Val->Pointer;
    int            pitch   = Param[2]->Val->Integer;

    sdl_update_texture(texture, pixels, pitch);
}

void Sdl_render_texture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    bool           xy_is_ctr = Param[0]->Val->Integer;
    int            x         = Param[1]->Val->Integer;
    int            y         = Param[2]->Val->Integer;
    sdl_texture_t *texture   = (sdl_texture_t*)Param[3]->Val->Pointer;

    sdl_render_texture(xy_is_ctr, x, y, texture);
}

void Sdl_render_scaled_texture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    bool           xy_is_ctr  = Param[0]->Val->Integer;
    int            x          = Param[1]->Val->Integer;
    int            y          = Param[2]->Val->Integer;
    int            w          = Param[3]->Val->Integer;
    int            h          = Param[4]->Val->Integer;
    sdl_texture_t *texture = (sdl_texture_t*)Param[5]->Val->Pointer;

    sdl_render_scaled_texture(xy_is_ctr, x, y, w, h, texture);
}

void Sdl_render_rotated_texture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    bool           xy_is_ctr = Param[0]->Val->Integer;
    int            x         = Param[1]->Val->Integer;
    int            y         = Param[2]->Val->Integer;
    double         angle     = Param[3]->Val->FP;
    sdl_texture_t *texture   = (sdl_texture_t*)Param[4]->Val->Pointer;

    sdl_render_rotated_texture(xy_is_ctr, x, y, angle, texture);
}

// -----------------  SDL PLATFORM REGISTRATION -------------------------

const char SdlDefs[] = "\
typedef struct { int x; int y; int w; int h; } sdl_rect_t; \n\
typedef struct { int x; int y; } sdl_point_t; \n\
typedef struct sdl_texture sdl_texture_t; \n\
\n\
#define BYTES_PER_PIXEL  4 \n\
#define COLOR_PURPLE     ( 127  |    0<<8 |  255<<16 |  255<<24 ) \n\
#define COLOR_BLUE       ( 0    |    0<<8 |  255<<16 |  255<<24 ) \n\
#define COLOR_LIGHT_BLUE ( 0    |  255<<8 |  255<<16 |  255<<24 ) \n\
#define COLOR_GREEN      ( 0    |  255<<8 |    0<<16 |  255<<24 ) \n\
#define COLOR_YELLOW     ( 255  |  255<<8 |    0<<16 |  255<<24 ) \n\
#define COLOR_ORANGE     ( 255  |  128<<8 |    0<<16 |  255<<24 ) \n\
#define COLOR_PINK       ( 255  |  105<<8 |  180<<16 |  255<<24 ) \n\
#define COLOR_RED        ( 255  |    0<<8 |    0<<16 |  255<<24 ) \n\
#define COLOR_GRAY       ( 224  |  224<<8 |  224<<16 |  255<<24 ) \n\
#define COLOR_WHITE      ( 255  |  255<<8 |  255<<16 |  255<<24 ) \n\
#define COLOR_BLACK      ( 0    |    0<<8 |    0<<16 |  255<<24 ) \n\
\n\
#define EVID_SWIPE_DOWN        9000 \n\
#define EVID_SWIPE_UP          9001 \n\
#define EVID_SWIPE_RIGHT       9002 \n\
#define EVID_SWIPE_LEFT        9003 \n\
#define EVID_QUIT              9999 \n\
";

struct LibraryFunction SdlFunctions[] = {
    // sdl initialization and termination, must be done once
    { Sdl_init,            "int sdl_init(void);" },
    { Sdl_exit,            "void sdl_exit(void);" },
    { Sdl_get_win_size,    "void sdl_get_win_size(int *w, int *h);" },

    // display init and present, must be done for every display update
    { Sdl_display_init,    "void sdl_display_init(int color);" },
    { Sdl_display_present, "void sdl_display_present(void);" },

    // event registration and query
    { Sdl_register_event,  "void sdl_register_event(sdl_rect_t *loc, int event_id);" },
    { Sdl_get_event,       "int sdl_get_event(long timeout_us);" },

    // create colors
    { Sdl_create_color,    "int sdl_create_color(int r, int g, int b, int a);" },
    { Sdl_scale_color,     "int sdl_scale_color(int color, double inten);" },
    { Sdl_wavelength_to_color, "int sdl_wavelength_to_color(int wavelength);" },

    // render text
    { Sdl_print_init,      "void sdl_print_init(double numchars, int fg_color, int bg_color, int *char_width, int *char_height, int *win_rows, int *win_cols);" },
    { Sdl_render_text,     "sdl_rect_t *sdl_render_text(bool xy_is_ctr, int x, int y, char *str);" },
    { Sdl_render_printf,   "sdl_rect_t *sdl_render_printf(bool xy_is_ctr, int x, int y, char *fmt, ...);" },

    // render rectangle, lines, circles, points
    { Sdl_render_rect,     "void sdl_render_rect(int xy_is_ctr, int x, int y, int w, int h, int line_width, int color);" },
    { Sdl_render_fill_rect,"void sdl_render_fill_rect(int xy_is_ctr, int x, int y, int w, int h, int color);" },
    { Sdl_render_line,     "void sdl_render_line(int x1, int y1, int x2, int y2, int color);" },
    { Sdl_render_lines,    "void sdl_render_lines(sdl_point_t *points, int count, int color);" },
    { Sdl_render_circle,   "void sdl_render_circle(bool xy_is_ctr, int x, int y, int radius, int line_width, int color);" },
    { Sdl_render_point,    "void sdl_render_point(int x, int y, int color, int point_size);" },
    { Sdl_render_points,   "void sdl_render_points(sdl_point_t *points, int count, int color, int point_size);" },

    // render using textures
    { Sdl_create_texture,               "sdl_texture_t *sdl_create_texture(int w, int h);" },
    { Sdl_create_texture_from_display,  "sdl_texture_t *sdl_create_texture_from_display(bool xy_is_ctr, int x, int y, int w, int h);" },
    { Sdl_create_filled_circle_texture, "sdl_texture_t *sdl_create_filled_circle_texture(int radius, int color);" },
    { Sdl_create_text_texture,          "sdl_texture_t *sdl_create_text_texture(char *str);" },
    { Sdl_destroy_texture,              "void sdl_destroy_texture(sdl_texture_t *texture);" },
    { Sdl_query_texture,                "void sdl_query_texture(sdl_texture_t *texture, int *width, int *height);" },
    { Sdl_update_texture,               "void sdl_update_texture(sdl_texture_t *texture, char *pixels, int pitch);" },
    { Sdl_render_texture,               "void sdl_render_texture(bool xy_is_ctr, int x, int y, sdl_texture_t *texture);" },
    { Sdl_render_scaled_texture,        "void sdl_render_scaled_texture(bool xy_is_ctr, int x, int y, int w, int h, sdl_texture_t *texture);" },
    { Sdl_render_rotated_texture,       "void sdl_render_rotated_texture(bool xy_is_ctr, int x, int y, double angle, sdl_texture_t *texture);" },

    { NULL, NULL } };

void SdlSetupFunction(Picoc *pc)
{
    // not used
}

void PlatformLibraryInit(Picoc *pc)
{
    IncludeRegister(
        pc, 
        "sdl.h", 
        SdlSetupFunction,
        SdlFunctions, 
        SdlDefs);
}
