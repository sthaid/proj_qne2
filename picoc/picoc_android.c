#include "picoc.h"

#define PICOC_STACK_SIZE (128000*4)

#include "picoc.h"
#include "pthread.h"

void *run_prog_helper(void *cx)
{
    Picoc pc;
    int StackSize = getenv("STACKSIZE") ? atoi(getenv("STACKSIZE")) : PICOC_STACK_SIZE;

    PicocInitialize(&pc, StackSize);

    if (PicocPlatformSetExitPoint(&pc)) {
        printf("EXIT jmp\n");
        PicocCleanup(&pc);
        //return pc.PicocExitValue;
        return NULL;
    }

    PicocPlatformScanFile(&pc, "/data/data/org.sthaid.qne2/files/hello.c");

    char *argv[10];
    argv[0] = "hello";
    PicocCallMain(&pc, 1, argv);

    printf("EXIT normal\n");
    PicocCleanup(&pc);

    //return pc.PicocExitValue;
    return NULL;
}

void run_prog(bool bg)
{
    pthread_t tid;

    if (!bg) {
        run_prog_helper(NULL);
    } else {
        pthread_create(&tid, NULL, run_prog_helper, NULL);
    }
}
