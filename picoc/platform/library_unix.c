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
    sdl_loc_t *loc      = (sdl_loc_t*)Param[0]->Val->Pointer;
    int        event_id = Param[1]->Val->Integer;

    sdl_register_event(loc, event_id);
}

void Sdl_get_event (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    long         timeout_us = Param[0]->Val->LongInteger;
    sdl_event_t *event      = Param[1]->Val->Pointer;

    sdl_get_event(timeout_us, event);
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

void Sdl_print_init (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    double numchars = Param[0]->Val->FP;
    int    fg_color = Param[1]->Val->Integer;
    int    bg_color = Param[2]->Val->Integer;

    sdl_print_init(numchars, fg_color, bg_color);
}

void Sdl_render_text (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int         x   = Param[0]->Val->Integer;
    int         y   = Param[1]->Val->Integer;
    char       *str = Param[2]->Val->Pointer;
    sdl_loc_t  *loc;

    loc = sdl_render_text(x, y, str);

    ReturnValue->Val->Pointer = loc;
}

void Sdl_render_printf (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int   x   = Param[0]->Val->Integer;
    int   y   = Param[1]->Val->Integer;
    char *fmt = Param[2]->Val->Pointer;

    struct StdVararg PrintfArgs;
    char             str[200] = "";
    sdl_loc_t       *loc;

    PrintfArgs.Param = Param + 2;
    PrintfArgs.NumArgs = NumArgs - 3;
    StdioBasePrintf(Parser, NULL, str, sizeof(str), fmt, &PrintfArgs);

    loc = sdl_render_text(x, y, str);

    ReturnValue->Val->Pointer = loc;
}

void Sdl_render_text_xyctr (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int         x   = Param[0]->Val->Integer;
    int         y   = Param[1]->Val->Integer;
    char       *str = Param[2]->Val->Pointer;
    sdl_loc_t  *loc;

    loc = sdl_render_text_xyctr(x, y, str);

    ReturnValue->Val->Pointer = loc;
}

void Sdl_render_printf_xyctr (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int   x   = Param[0]->Val->Integer;
    int   y   = Param[1]->Val->Integer;
    char *fmt = Param[2]->Val->Pointer;

    struct StdVararg PrintfArgs;
    char             str[200] = "";
    sdl_loc_t       *loc;

    PrintfArgs.Param = Param + 2;
    PrintfArgs.NumArgs = NumArgs - 3;
    StdioBasePrintf(Parser, NULL, str, sizeof(str), fmt, &PrintfArgs);

    loc = sdl_render_text_xyctr(x, y, str);

    ReturnValue->Val->Pointer = loc;
}

void Sdl_render_multiline_text (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int   y_top           = Param[0]->Val->Integer;
    int   y_display_begin = Param[1]->Val->Integer;
    int   y_display_end   = Param[2]->Val->Integer;
    char *str             = Param[3]->Val->Pointer;

    sdl_render_multiline_text(y_top, y_display_begin, y_display_end, str);
}

void Sdl_render_multiline_text_2 (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int    y_top           = Param[0]->Val->Integer;
    int    y_display_begin = Param[1]->Val->Integer;
    int    y_display_end   = Param[2]->Val->Integer;
    char **lines           = Param[3]->Val->Pointer;
    int    n               = Param[4]->Val->Integer;

    sdl_render_multiline_text_2(y_top, y_display_begin, y_display_end, lines, n);
}

//
// render rectangle, lines, circles, points
//

void Sdl_render_rect (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int   x          = Param[0]->Val->Integer;
    int   y          = Param[1]->Val->Integer;
    int   w          = Param[2]->Val->Integer;
    int   h          = Param[3]->Val->Integer;
    int   line_width = Param[4]->Val->Integer;
    int   color      = Param[5]->Val->Integer;

    sdl_render_rect(x, y, w, h, line_width, color);
}

void Sdl_render_fill_rect (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int   x          = Param[0]->Val->Integer;
    int   y          = Param[1]->Val->Integer;
    int   w          = Param[2]->Val->Integer;
    int   h          = Param[3]->Val->Integer;
    int   color      = Param[4]->Val->Integer;

    sdl_render_fill_rect(x, y, w, h, color);
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
    int  x_ctr      = Param[0]->Val->Integer;
    int  y_ctr      = Param[1]->Val->Integer;
    int  radius     = Param[2]->Val->Integer;
    int  line_width = Param[3]->Val->Integer;
    int  color      = Param[4]->Val->Integer;

    sdl_render_circle(x_ctr, y_ctr, radius, line_width, color);
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

void Sdl_create_texture(struct ParseState *Parser, struct Value *ReturnValue,
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
    int x = Param[0]->Val->Integer;
    int y = Param[1]->Val->Integer;
    int w = Param[2]->Val->Integer;
    int h = Param[3]->Val->Integer;

    sdl_texture_t *texture;

    texture = sdl_create_texture_from_display(x, y, w, h);
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

void Sdl_render_texture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int            x       = Param[0]->Val->Integer;
    int            y       = Param[1]->Val->Integer;
    int            w       = Param[2]->Val->Integer;
    int            h       = Param[3]->Val->Integer;
    double         angle   = Param[4]->Val->FP;
    sdl_texture_t *texture = (sdl_texture_t*)Param[5]->Val->Pointer;

    sdl_render_texture(x, y, w, h, angle, texture);
}

void Sdl_destroy_texture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_texture_t *texture = (sdl_texture_t*)Param[0]->Val->Pointer;

    sdl_destroy_texture(texture);
}

void Sdl_update_texture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_texture_t *texture = (sdl_texture_t*)Param[0]->Val->Pointer;
    int           *pixels  = (int*)Param[1]->Val->Pointer;

    sdl_update_texture(texture, pixels);
}

void Sdl_query_texture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_texture_t *texture = (sdl_texture_t*)Param[0]->Val->Pointer;
    int           *width   = (int*)Param[1]->Val->Pointer;
    int           *height  = (int*)Param[2]->Val->Pointer;

    sdl_query_texture(texture, width, height);
}

// -----------------  SDL PLATFORM REGISTRATION -------------------------

// xxx reformat
const char SdlDefs[] = "\
typedef struct { int x; int y; int w; int h; } sdl_loc_t; \n\
typedef struct { int x; int y; } sdl_point_t; \n\
typedef struct sdl_texture sdl_texture_t; \n\
typedef struct { \n\
    int event_id; \n\
    union { \n\
        struct { \n\
            int x; int y; int xrel; int yrel; \n\
        } motion; \n\
    } u; \n\
} sdl_event_t; \n\
\n\
#define BYTES_PER_PIXEL  4 \n\
#define COLOR_BLACK      (   0  |    0<<8 |    0<<16 |  255<<24 ) \n\
#define COLOR_WHITE      ( 255  |  255<<8 |  255<<16 |  255<<24 ) \n\
#define COLOR_RED        ( 255  |    0<<8 |    0<<16 |  255<<24 ) \n\
#define COLOR_ORANGE     ( 255  |  128<<8 |    0<<16 |  255<<24 ) \n\
#define COLOR_YELLOW     ( 255  |  255<<8 |    0<<16 |  255<<24 ) \n\
#define COLOR_GREEN      (   0  |  255<<8 |    0<<16 |  255<<24 ) \n\
#define COLOR_BLUE       (   0  |    0<<8 |  255<<16 |  255<<24 ) \n\
#define COLOR_INDIGO     (  75  |    0<<8 |  130<<16 |  255<<24 ) \n\
#define COLOR_VIOLET     ( 238  |  130<<8 |  238<<16 |  255<<24 ) \n\
#define COLOR_PURPLE     ( 127  |    0<<8 |  255<<16 |  255<<24 ) \n\
#define COLOR_LIGHT_BLUE (   0  |  255<<8 |  255<<16 |  255<<24 ) \n\
#define COLOR_PINK       ( 255  |  105<<8 |  180<<16 |  255<<24 ) \n\
#define COLOR_TEAL       (   0  |  128<<8 |  128<<16 |  255<<24 ) \n\
#define COLOR_LIGHT_GRAY ( 192  |  192<<8 |  192<<16 |  255<<24 ) \n\
#define COLOR_GRAY       ( 128  |  128<<8 |  128<<16 |  255<<24 ) \n\
#define COLOR_DARK_GRAY  (  64  |   64<<8 |   64<<16 |  255<<24 ) \n\
\n\
#define EVID_SWIPE_RIGHT       9990 \n\
#define EVID_SWIPE_LEFT        9991 \n\
#define EVID_MOTION            9992 \n\
#define EVID_QUIT              9999 \n\
";

struct LibraryFunction SdlFunctions[] = {
    // sdl initialization and termination, must be done once
    { Sdl_init,            "int sdl_init(void);" },
    { Sdl_exit,            "void sdl_exit(void);" },

    // display init and present, must be done for every display update
    { Sdl_display_init,    "void sdl_display_init(int color);" },
    { Sdl_display_present, "void sdl_display_present(void);" },

    // event registration and query
    { Sdl_register_event,  "void sdl_register_event(sdl_loc_t *loc, int event_id);" },
    { Sdl_get_event,       "void sdl_get_event(long timeout_us, sdl_event_t *event);" },

    // create colors
    { Sdl_create_color,    "int sdl_create_color(int r, int g, int b, int a);" },
    { Sdl_scale_color,     "int sdl_scale_color(int color, double inten);" },
    { Sdl_wavelength_to_color, "int sdl_wavelength_to_color(int wavelength);" },

    // render text
    { Sdl_print_init,              "void sdl_print_init(double numchars, int fg_color, int bg_color);" },
    { Sdl_render_text,             "sdl_loc_t *sdl_render_text(int x, int y, char *str);" },
    { Sdl_render_printf,           "sdl_loc_t *sdl_render_printf(int x, int y, char *fmt, ...);" },
    { Sdl_render_text_xyctr,       "sdl_loc_t *sdl_render_text_xyctr(int x, int y, char *str);" },
    { Sdl_render_printf_xyctr,     "sdl_loc_t *sdl_render_printf_xyctr(int x, int y, char *fmt, ...);" },
    { Sdl_render_multiline_text,   "void sdl_render_multiline_text(int y_top, int y_display_begin, int y_display_end, char * str);" },
    { Sdl_render_multiline_text_2, "void sdl_render_multiline_text_2(int y_top, int y_display_begin, int y_display_end, char **lines, int n);" },

    // render rectangle, lines, circles, points
    { Sdl_render_rect,     "void sdl_render_rect(int x, int y, int w, int h, int line_width, int color);" },
    { Sdl_render_fill_rect,"void sdl_render_fill_rect(int x, int y, int w, int h, int color);" },
    { Sdl_render_line,     "void sdl_render_line(int x1, int y1, int x2, int y2, int color);" },
    { Sdl_render_lines,    "void sdl_render_lines(sdl_point_t *points, int count, int color);" },
    { Sdl_render_circle,   "void sdl_render_circle(int x_ctr, int y_ctr, int radius, int line_width, int color);" },
    { Sdl_render_point,    "void sdl_render_point(int x, int y, int color, int point_size);" },
    { Sdl_render_points,   "void sdl_render_points(sdl_point_t *points, int count, int color, int point_size);" },

    // render using textures
    { Sdl_create_texture,               "sdl_texture_t *sdl_create_texture(int w, int h);" },
    { Sdl_create_texture_from_display,  "sdl_texture_t *sdl_create_texture_from_display(int x, int y, int w, int h);" },
    { Sdl_create_filled_circle_texture, "sdl_texture_t *sdl_create_filled_circle_texture(int radius, int color);" },
    { Sdl_create_text_texture,          "sdl_texture_t *sdl_create_text_texture(char *str);" },
    { Sdl_render_texture,               "void sdl_render_texture(int x, int y, int w, int h, double angle, sdl_texture_t *texture);" },
    { Sdl_destroy_texture,              "void sdl_destroy_texture(sdl_texture_t *texture);" },
    { Sdl_update_texture,               "void sdl_update_texture(sdl_texture_t *texture, int *pixels);" },
    { Sdl_query_texture,                "void sdl_query_texture(sdl_texture_t *texture, int *width, int *height);" },

    { NULL, NULL } };

void SdlSetupFunction(Picoc *pc)
{
    #define PLATFORM_VAR(name, type, writeable) \
        do { \
            VariableDefinePlatformVar(pc, NULL, #name, &pc->type, \
                                      (union AnyValue *)&name, writeable); \
        } while (0)
        
    PLATFORM_VAR(sdl_win_width, IntType, false);
    PLATFORM_VAR(sdl_win_height, IntType, false);
    PLATFORM_VAR(sdl_char_width, IntType, false);
    PLATFORM_VAR(sdl_char_height, IntType, false);
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
