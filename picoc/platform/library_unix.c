#include "../interpreter.h"
#include "../../sdl.h"

void UnixSetupFunc(Picoc *pc)
{
    printf("XXXXXXX UnixSetupFunc ...\n");

// TypeCreateOpaqueStruct(pc, NULL, TableStrRegister(pc, "sdl_rect"),
//      sizeof(struct sdl_rect));


    // xxx or move to bottom
    #define XXX(name, type, writeable) \
        VariableDefinePlatformVar(pc, \
                                  NULL, \
                                  #name, \
                                  &pc->type,\
                                  (union AnyValue*)&name,\
                                  writeable);
        
    //XXX(COLOR_YELLOW, IntType, false);

    //VariableDefinePlatformVar(pc, NULL, "COLOR_YELLOW", &pc->IntType,
        //(union AnyValue*)&COLOR_YELLOW, false);
}

void Ctest (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    printf("test(%d)\n", Param[0]->Val->Integer);
}

void Clineno (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = Parser->Line;
}

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

void Test2 (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    static struct sdl_rect loc = {1,2,3,7};

    ReturnValue->Val->Pointer = &loc;
}

void Sdl_render_text (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    static struct sdl_rect loc;

    int x = Param[0]->Val->Integer;
    int y = Param[1]->Val->Integer;
    char *str = Param[2]->Val->Identifier;

    printf("XXX GOT %d %d %s\n", x, y, str);

    loc = *sdl_render_text(x, y, str);

    ReturnValue->Val->Pointer = &loc;
}

struct StdVararg
{
    struct Value **Param;
    int NumArgs;
};

int StdioBasePrintf(struct ParseState *Parser, FILE *Stream, char *StrOut,
    int StrOutLen, char *Format, struct StdVararg *Args);

void Sdl_render_printf (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    struct StdVararg PrintfArgs;
    int x =  Param[0]->Val->Integer;
    int y =  Param[1]->Val->Integer;
    char *fmt = Param[2]->Val->Identifier;
    char str[200] = "";

    static struct sdl_rect loc;

    PrintfArgs.Param = Param + 2;
    PrintfArgs.NumArgs = NumArgs - 3;

    StdioBasePrintf(Parser, NULL, str, sizeof(str), fmt, &PrintfArgs);

    printf("XXXXX NumArgs=%d xy=%d %d fmt=%s str=%s\n", NumArgs, x, y, fmt, str);

    loc = *sdl_render_text(x, y, str);

    ReturnValue->Val->Pointer = &loc;
}

/* list of all library functions and their prototypes */
struct LibraryFunction UnixFunctions[] =
{
    {Ctest, "void test(int);"},
    {Clineno, "int lineno();"},

    { Sdl_display_init, "void sdl_display_init(int color);" },
    { Sdl_display_present, "void sdl_display_present(void);" },
    { Sdl_render_text, "struct sdl_rect *sdl_render_text(int x, int y, char * str);" },
    { Sdl_render_printf, "struct sdl_rect *sdl_render_printf(int x, int y, char * fmt, ...) ;" },

    { Test2, "struct sdl_rect *test2(void);" },

    {NULL, NULL}
};

void PlatformLibraryInit(Picoc *pc)
{
    XXX(COLOR_YELLOW, IntType, false);

    //IncludeRegister(pc, "picoc_unix.h", &UnixSetupFunc, &UnixFunctions[0], NULL);
    IncludeRegister(pc, "picoc_unix.h", &UnixSetupFunc, &UnixFunctions[0], 
      "struct sdl_rect {short x; short y; short w; short h;};" );
}
