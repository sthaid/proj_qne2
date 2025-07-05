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
    int               x   = Param[0]->Val->Integer;
    int               y   = Param[1]->Val->Integer;
    char            * str = Param[2]->Val->Identifier;
    struct sdl_rect * loc;

    loc = sdl_render_text(x, y, str);

    ReturnValue->Val->Pointer = loc;
}

void Sdl_render_printf (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int               x   =  Param[0]->Val->Integer;
    int               y   =  Param[1]->Val->Integer;
    char            * fmt = Param[2]->Val->Identifier;
    struct StdVararg  PrintfArgs;
    char              str[200] = "";
    struct sdl_rect * loc;

    PrintfArgs.Param = Param + 2;
    PrintfArgs.NumArgs = NumArgs - 3;
    StdioBasePrintf(Parser, NULL, str, sizeof(str), fmt, &PrintfArgs);

    loc = sdl_render_text(x, y, str);

    ReturnValue->Val->Pointer = loc;
}

// -----------------  SDL PLATFORM REGISTRATION -------------------------

struct LibraryFunction SdlFunctions[] = {
    { Sdl_display_init,    "void sdl_display_init(int color);" },
    { Sdl_display_present, "void sdl_display_present(void);" },
    { Sdl_render_text,     "struct sdl_rect *sdl_render_text(int x, int y, char * str);" },
    { Sdl_render_printf,   "struct sdl_rect *sdl_render_printf(int x, int y, char * fmt, ...) ;" },
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
        "struct sdl_rect {short x; short y; short w; short h;};"
                    );
}
