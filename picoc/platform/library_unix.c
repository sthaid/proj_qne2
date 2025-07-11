#include "../interpreter.h"
#include "../../sdl.h"

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
    int *w = (int*)Param[0]->Val->Pointer;
    int *h = (int*)Param[1]->Val->Pointer;
    int ret;

    ret = sdl_init(w, h);

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
    int inten = Param[1]->Val->FP;
    int scaled_color;

    scaled_color = sdl_scale_color(color, inten);

    ReturnValue->Val->Integer = scaled_color;
}

//
// render text
//

void Sdl_render_text (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int         x        = Param[0]->Val->Integer;
    int         y        = Param[1]->Val->Integer;
    int         ptsize   = Param[2]->Val->Integer;
    int         fg_color = Param[3]->Val->Integer;
    int         bg_color = Param[4]->Val->Integer;
    char       *str      = Param[5]->Val->Pointer;
    sdl_rect_t *loc;

    loc = sdl_render_text(x, y, ptsize, fg_color, bg_color, str);

    ReturnValue->Val->Pointer = loc;
}

void Sdl_render_printf (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int              x        = Param[0]->Val->Integer;
    int              y        = Param[1]->Val->Integer;
    int              ptsize   = Param[2]->Val->Integer;
    int              fg_color = Param[3]->Val->Integer;
    int              bg_color = Param[4]->Val->Integer;
    char            *fmt      = Param[5]->Val->Pointer;

    struct StdVararg PrintfArgs;
    char             str[200] = "";
    sdl_rect_t      *loc;

    PrintfArgs.Param = Param + 5;
    PrintfArgs.NumArgs = NumArgs - 6;
    StdioBasePrintf(Parser, NULL, str, sizeof(str), fmt, &PrintfArgs);

    loc = sdl_render_text(x, y, ptsize, fg_color, bg_color, str);

    ReturnValue->Val->Pointer = loc;
}

void Sdl_get_char_size (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int  ptsize       = Param[0]->Val->Integer;
    int *char_width   = (int*)Param[1]->Val->Pointer;
    int *char_height  = (int*)Param[2]->Val->Pointer;

    sdl_get_char_size(ptsize, char_width, char_height);
}

//
// render rectangle, lines, circles, points
//

void Sdl_render_rect (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_rect_t *loc        = (sdl_rect_t*)Param[0]->Val->Pointer;
    int         line_width = Param[1]->Val->Integer;
    int         color      = Param[2]->Val->Integer;

    sdl_render_rect(loc, line_width, color);
}

void Sdl_render_fill_rect (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_rect_t *loc   = (sdl_rect_t*)Param[0]->Val->Pointer;
    int         color = Param[1]->Val->Integer;

    sdl_render_fill_rect(loc, color);
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
    int x_center    = Param[0]->Val->Integer;
    int y_center    = Param[1]->Val->Integer;
    int radius      = Param[2]->Val->Integer;
    int line_width  = Param[3]->Val->Integer;
    int color       = Param[4]->Val->Integer;

    sdl_render_circle(x_center, y_center, radius, line_width, color);
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
    sdl_rect_t *loc = (sdl_rect_t*)Param[0]->Val->Pointer;
    sdl_texture_t *texture;

    texture = sdl_create_texture_from_display(loc);
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
    int   ptsize   = Param[0]->Val->Integer;
    int   fg_color = Param[1]->Val->Integer;
    int   bg_color = Param[2]->Val->Integer;
    char *str      = (char*)Param[3]->Val->Pointer;
    sdl_texture_t *texture;

    texture = sdl_create_text_texture(ptsize, fg_color, bg_color, str);
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
    int            x       = Param[0]->Val->Integer;
    int            y       = Param[1]->Val->Integer;
    sdl_texture_t *texture = (sdl_texture_t*)Param[2]->Val->Pointer;

    sdl_render_texture(x, y, texture);
}

void Sdl_render_scaled_texture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_rect_t    *dest    = (sdl_rect_t*)Param[0]->Val->Pointer;
    sdl_texture_t *texture = (sdl_texture_t*)Param[1]->Val->Pointer;

    sdl_render_scaled_texture(dest, texture);
}

// -----------------  SDL PLATFORM REGISTRATION -------------------------

struct LibraryFunction SdlFunctions[] = {
    // sdl initialization and termination, must be done once
    { Sdl_init,            "int sdl_init(int *w, int *h);" },
    { Sdl_exit,            "void sdl_exit(void);" },

    // display init and present, must be done for every display update
    { Sdl_display_init,    "void sdl_display_init(int color);" },
    { Sdl_display_present, "void sdl_display_present(void);" },

    // event registration and query
    { Sdl_register_event,  "void sdl_register_event(sdl_rect_t *loc, int event_id);" },
    { Sdl_get_event,       "int sdl_get_event(long timeout_us);" },

    // create colors
    { Sdl_create_color,    "int sdl_create_color(int r, int g, int b, int a);" },
    { Sdl_scale_color,     "int sdl_scale_color(int color, double inten);" },

    // render text
    { Sdl_render_text,     "sdl_rect_t *sdl_render_text(int x, int y, int ptsize, int fg_color, int bg_color, char *str);" },
    { Sdl_render_printf,   "sdl_rect_t *sdl_render_printf(int x, int y, int ptsize, int fg_color, int bg_color, char *fmt, ...);" },
    { Sdl_get_char_size,   "void sdl_get_char_size(int ptsize, int *char_width, int *char_height);" },

    // render rectangle, lines, circles, points
    { Sdl_render_rect,     "void sdl_render_rect(sdl_rect_t *loc, int line_width, int color);" },
    { Sdl_render_fill_rect,"void sdl_render_fill_rect(sdl_rect_t *loc, int color);" },
    { Sdl_render_line,     "void sdl_render_line(int x1, int y1, int x2, int y2, int color);" },
    { Sdl_render_lines,    "void sdl_render_lines(sdl_point_t *points, int count, int color);" },
    { Sdl_render_circle,   "void sdl_render_circle(int x_center, int y_center, int radius, int line_width, int color);" },
    { Sdl_render_point,    "void sdl_render_point(int x, int y, int color, int point_size);" },
    { Sdl_render_points,   "void sdl_render_points(sdl_point_t *points, int count, int color, int point_size);" },

    // render using textures
    { Sdl_create_texture,               "sdl_texture_t *sdl_create_texture(int w, int h);" },
    { Sdl_create_texture_from_display,  "sdl_texture_t *sdl_create_texture_from_display(sdl_rect_t *display);" },
    { Sdl_create_filled_circle_texture, "sdl_texture_t *sdl_create_filled_circle_texture(int radius, int color);" },
    { Sdl_create_text_texture,          "sdl_texture_t *sdl_create_text_texture(int ptsize, int fg_color, int bg_color, char *str);" },
    { Sdl_destroy_texture,              "void sdl_destroy_texture(sdl_texture_t *texture);" },
    { Sdl_query_texture,                "void sdl_query_texture(sdl_texture_t *texture, int *width, int *height);" },
    { Sdl_update_texture,               "void sdl_update_texture(sdl_texture_t *texture, char *pixels, int pitch);" },
    { Sdl_render_texture,               "void sdl_render_texture(int x, int y, sdl_texture_t *texture);" },
    { Sdl_render_scaled_texture,        "void sdl_render_scaled_texture(sdl_rect_t *dest, sdl_texture_t *texture);" },

    { NULL, NULL } };


const char SdlDefs[] = "\
typedef struct { short x; short y; short w; short h; } sdl_rect_t; \
typedef struct { int x; int y; } sdl_point_t; \
typedef struct sdl_texture sdl_texture_t; \
";

void PlatformLibraryInit(Picoc *pc)
{
    #define DEFINE_PLATFORM_VAR(name, type, writeable) \
        VariableDefinePlatformVar(pc, \
                                  NULL, \
                                  #name, \
                                  &pc->type,\
                                  (union AnyValue*)&name,\
                                  writeable);

    // sdl - platform init xxx
    DEFINE_PLATFORM_VAR(COLOR_PURPLE, IntType, false);
    DEFINE_PLATFORM_VAR(COLOR_BLUE, IntType, false);
    DEFINE_PLATFORM_VAR(COLOR_LIGHT_BLUE, IntType, false);
    DEFINE_PLATFORM_VAR(COLOR_GREEN, IntType, false);
    DEFINE_PLATFORM_VAR(COLOR_YELLOW, IntType, false);
    DEFINE_PLATFORM_VAR(COLOR_ORANGE, IntType, false);
    DEFINE_PLATFORM_VAR(COLOR_PINK, IntType, false);
    DEFINE_PLATFORM_VAR(COLOR_RED, IntType, false);
    DEFINE_PLATFORM_VAR(COLOR_GRAY, IntType, false);
    DEFINE_PLATFORM_VAR(COLOR_WHITE, IntType, false);
    DEFINE_PLATFORM_VAR(COLOR_BLACK, IntType, false);

    IncludeRegister(
        pc, 
        "sdl.h", 
        NULL,  // SetupFunction not used
        SdlFunctions, 
        SdlDefs
                    );
}


