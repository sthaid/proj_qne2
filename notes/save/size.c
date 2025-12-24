#include <stdio.h>

#define IS_UNSIGNED(T) (((T)-1) > 0)
#define IS_SIGNED(T) (!IS_UNSIGNED(T))

#define TEST_EXPR(expr) \
    do { \
        printf("%-30s VALUE = %08X  SIZE = %zd  %s\n", #expr, expr, sizeof(expr),  \
            IS_SIGNED(typeof(expr)) ? "SIGNED" : "UNSIGNED"); \
    } while (0)

#define TEST_EXPR64(expr) \
    do { \
        printf("%-30s VALUE = %08lX  SIZE = %zd  %s\n", #expr, expr, sizeof(expr),  \
            IS_SIGNED(typeof(expr)) ? "SIGNED" : "UNSIGNED"); \
    } while (0)

int main()
{
    signed char sch=-1;
    unsigned char uch=-1;

    signed short sshort=-1;
    unsigned short ushort=-1;

    signed int sint=-1;
    unsigned int uint=-1;

    signed long slong=-1;
    unsigned long ulong=-1;

    TEST_EXPR(uch << 1);
    TEST_EXPR(ushort << 1);
    TEST_EXPR(uint << 1);
    TEST_EXPR64(ulong << 1);

    TEST_EXPR(1<<2);
    TEST_EXPR(1u<<2);
    TEST_EXPR(1 * 2);
    TEST_EXPR(1u * 2);
    TEST_EXPR(1 * 2u);

    TEST_EXPR(sch);
    TEST_EXPR(uch);
    TEST_EXPR(sch * 2);
    TEST_EXPR(uch * 2);

    TEST_EXPR(sshort);
    TEST_EXPR(ushort);
    TEST_EXPR(sshort * 2);
    TEST_EXPR(ushort * 2);
    
    TEST_EXPR(sshort / 2);
    TEST_EXPR(ushort / 2);

    TEST_EXPR(sshort << 2);
    TEST_EXPR(ushort << 2);

    TEST_EXPR(sint << 2);
    TEST_EXPR(uint << 2);

    TEST_EXPR(1 * (char)2);
    TEST_EXPR(1 * (short)2);
    TEST_EXPR(1 * (int)2);
    TEST_EXPR64(1 * (long)2);

    TEST_EXPR(1 * (unsigned char)2);
    TEST_EXPR(1 * (unsigned short)2);
    TEST_EXPR(1 * (unsigned int)2);
    TEST_EXPR64(1 * (unsigned long)2);

    TEST_EXPR(uch * sint);
    TEST_EXPR(sch * uint);

    TEST_EXPR64(uch * slong);
    TEST_EXPR64(sch * ulong);

    TEST_EXPR(ushort *ushort);
    TEST_EXPR64(slong *uint);
    TEST_EXPR64(ulong *ulong);
    TEST_EXPR64(ulong *slong);
    TEST_EXPR64(ulong *sint);
    TEST_EXPR64(ulong *sshort);
    TEST_EXPR64(ulong *sch);

    TEST_EXPR(uint * sch);
    TEST_EXPR(uint * uch);
    TEST_EXPR(uint * sshort);
    TEST_EXPR(uint * ushort);
    TEST_EXPR(uint * sint);
    TEST_EXPR(uint * uint);
    TEST_EXPR64(uint * slong);
    TEST_EXPR64(uint * ulong);

    TEST_EXPR64(uint | slong);
    TEST_EXPR64(uint | ulong);

    return 0;
}
