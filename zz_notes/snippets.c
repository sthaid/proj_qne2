cstdlib/stdio.c

// EZAPP TODO
#if 0
    // xxxxxxxxxxxxx
    {
    printf("XXX FORMAT '%s'  NUM_ARGS=%d\n", Format, Args->NumArgs);
    int num_args = Args->NumArgs;
    struct Value *this_arg = Args->Param[0];
    unsigned long x[10];
    int char_count;

    memset(x, 0, sizeof(x));

    for (int i = 0; i < num_args; i++) {
        this_arg = (struct Value*)((char*)this_arg +
                            MEM_ALIGN(sizeof(struct Value)+TypeStackSizeValue(this_arg)));
        x[i] = this_arg->Val->LongInteger;
        printf("%d : %lx\n", i, x[i]);
    }
    char_count = printf(Format, 
           x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7], x[8], x[9]);
    printf("---------------\n");
    return char_count;  
    }
#endif
