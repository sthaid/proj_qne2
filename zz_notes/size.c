#include <stdio.h>

#define TEST(expr) \
    do { \
        printf("%-30s %zd\n", #expr, sizeof(expr)); \
    } while (0)

int main()
{
#if 0
    TEST(1);
    TEST(1L);
    TEST(1L << 2);

    TEST(1 | 2);
    TEST(1UL | 2);
    TEST(1L | 2);
    TEST(1L | 2L);

    TEST(signed);
    TEST(unsigned);

    TEST(signed char);
    TEST(unsigned char);
    TEST(signed short);
    TEST(unsigned short);
    TEST(signed int);
    TEST(unsigned int);
    TEST(signed long);
    TEST(unsigned long);
#endif

    signed char sch=-1;
    unsigned char uch=-1;

    signed short sshort=-1;
    unsigned short ushort=-1;

    signed int sint=-1;
    unsigned int uint=-1;
//  TEST(ch);
//  TEST((ch));
//  TEST(ch * ch);
//  TEST(ch / ch);
//  TEST(ch << 7);

#define IS_UNSIGNED(T) (((T)-1) > 0)
#define IS_SIGNED(T) (!IS_UNSIGNED(T))

#define TEST_EXPR(expr) \
    do { \
        printf("%-30s VALUE = %08X  SIZE = %zd  %s\n", #expr, expr, sizeof(expr),  \
            IS_SIGNED(typeof(expr)) ? "SIGNED" : "UNSIGNED"); \
    } while (0)

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
    TEST_EXPR(1 * (long)2);

    TEST_EXPR(1 * (unsigned char)2);
    TEST_EXPR(1 * (unsigned short)2);
    TEST_EXPR(1 * (unsigned int)2);
    TEST_EXPR(1 * (unsigned long)2);

    return 0;
}
