#include <libgen.h>
#include <pthread.h>

#include "picoc.h"

#define PICOC_STACK_SIZE (128000*4)  // xxx check this

static int picoc_helper(char *args);
//static void *picoc_thread(void *cx);

// ----------------- API: RUN PICOC PROG -------------

int picoc_fg(char *args)
{
    return picoc_helper(args);
}

#if 0 //xxx cleanup
void picoc_bg(char *args)
{
    pthread_t tid;

    pthread_create(&tid, NULL, picoc_thread, args); //xxx detached
}
#endif

// ----------------- SUPPORT -------------------------

#if 0
// xxx should use sdl thread
static void *picoc_thread(void *cx)
{
    char *args = (char*)cx;
    int rc;

    rc = picoc_helper(args);

    return (void*)(long)rc;
}
#endif

static int picoc_helper(char *args)
{
    Picoc pc;
    char  args_copy[10000];  // xxx malloc
    char *argv[20];
    char *s;
    int   argc = 0;
    bool  processing_argv_args = false;
    bool  first = true;
    char *saveptr;

    // init pc
    PicocInitialize(&pc, PICOC_STACK_SIZE);

    // setjmp for error condition
    if (PicocPlatformSetExitPoint(&pc)) {
        printf("EXIT jmp, %d\n", pc.PicocExitValue);
        PicocCleanup(&pc);
        return pc.PicocExitValue;
    }

    // tokenize args
    strcpy(args_copy, args);
    while (true) {
        s = strtok_r(first ? args_copy : NULL, " ", &saveptr);
        first = false;
        if (s == NULL) {
            break;
        }

        if (strcmp(s, "-") == 0) {
            processing_argv_args = true;
            continue;
        }

        if (!processing_argv_args) {
            printf("scanning %s\n", s);
            PicocPlatformScanFile(&pc, s);
        } else {
            printf("adding argv[%d] = %s\n", argc, s);
            argv[argc++] = s;
        }
    }
    
    // run program
    PicocCallMain(&pc, argc, argv);

// xxx callback with the completion
    // cleanup and return
    printf("EXIT normal, %d\n", pc.PicocExitValue);
    PicocCleanup(&pc);
    return pc.PicocExitValue;
}
    
