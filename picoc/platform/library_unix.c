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

void Sdl_render_text (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int               x        = Param[0]->Val->Integer;
    int               y        = Param[1]->Val->Integer;
    int               ptsize   = Param[2]->Val->Integer;
    int               fg_color = Param[3]->Val->Integer;
    int               bg_color = Param[4]->Val->Integer;
    char            * str      = Param[5]->Val->Identifier;
    sdl_rect_t * loc;

    loc = sdl_render_text(x, y, ptsize, fg_color, bg_color, str);

    ReturnValue->Val->Pointer = loc;
}

void Sdl_render_printf (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int               x        = Param[0]->Val->Integer;
    int               y        = Param[1]->Val->Integer;
    int               ptsize   = Param[2]->Val->Integer;
    int               fg_color = Param[3]->Val->Integer;
    int               bg_color = Param[4]->Val->Integer;
    char            * fmt      = Param[5]->Val->Identifier;
    struct StdVararg  PrintfArgs;
    char              str[200] = "";
    sdl_rect_t      * loc;

    PrintfArgs.Param = Param + 5;
    PrintfArgs.NumArgs = NumArgs - 6;
    StdioBasePrintf(Parser, NULL, str, sizeof(str), fmt, &PrintfArgs);

    loc = sdl_render_text(x, y, ptsize, fg_color, bg_color, str);

    ReturnValue->Val->Pointer = loc;
}

void Sdl_init (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int * w = (int*)Param[0]->Val->Identifier;
    int * h = (int*)Param[1]->Val->Identifier;
    int ret;

    ret = sdl_init(w, h);

    ReturnValue->Val->Integer = ret;
}

void Sdl_exit (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_exit();
}

void Sdl_render_circle (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int x_center        = Param[0]->Val->Integer;
    int y_center        = Param[1]->Val->Integer;
    int radius          = Param[2]->Val->Integer;
    int line_width      = Param[3]->Val->Integer;
    int color           = Param[4]->Val->Integer;

    sdl_render_circle(x_center, y_center, radius, line_width, color);
}

// -----------------  SDL PLATFORM REGISTRATION -------------------------

const char xxx[] = "\
typedef struct {short x; short y; short w; short h;} sdl_rect_t; \
";

struct LibraryFunction SdlFunctions[] = {
    { Sdl_init,            "int sdl_init(int *w, int *h);" },
    { Sdl_exit,            "void sdl_exit(void);" },

    { Sdl_display_init,    "void sdl_display_init(int color);" },
    { Sdl_display_present, "void sdl_display_present(void);" },

    { Sdl_render_text,     "sdl_rect_t *sdl_render_text(int x, int y, int ptsize, int fg_color, int bg_color, char * str);" },
    { Sdl_render_printf,   "sdl_rect_t *sdl_render_printf(int x, int y, int ptsize, int fg_color, int bg_color, char * fmt, ...) ;" },

    { Sdl_render_circle,  "void sdl_render_circle(int x_center, int y_center, int radius, int line_width, int color);" },

    { NULL, NULL } };

void PlatformLibraryInit(Picoc *pc)
{
    #define DEFINE_PLATFORM_VAR(name, type, writeable) \
        VariableDefinePlatformVar(pc, \
                                  NULL, \
                                  #name, \
                                  &pc->type,\
                                  (union AnyValue*)&name,\
                                  writeable);

    // sdl - platform init
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
        xxx
                    );
}


