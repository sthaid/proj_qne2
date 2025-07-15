#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>  // xxx limit these
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <math.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>

#include <sdl.h>

#define INFO printf
#define FATAL printf  //xxx

//
// defines
//

#define NONE   0
#define BLACK  1
#define WHITE  2

#define MOVE_PASS       -1
#define MOVE_GAME_OVER  -2
//#define MOVE_NONE       -9

#define OTHER_COLOR(c) ((c) == WHITE ? BLACK : WHITE)

// xxx can these be in reversi.c
#define MOVE_TO_RC(m,r,c) \
    do { \
        (r) = (m) / 10; \
        (c) = (m) % 10; \
    } while (0)

#define RC_TO_MOVE(r,c,m) \
    do { \
        (m) = ((r) * 10) + (c); \
    } while (0)

//
// typedefs
//

typedef struct {
    unsigned char pos[10][10];
    int black_cnt;
    int white_cnt;
    int whose_turn;
} board_t;

typedef struct {
    int move[64];
    int max;
    int color;
} possible_moves_t;

//
// variables
//

//
// prototypes
//

// main.c 
void get_possible_moves(board_t *b, possible_moves_t *pm);
// human.c
int human_get_move(board_t *b);
// cpu.c
int cpu_get_move(board_t *b);

//
// inline procedures
//

#if 0
static inline int min(int a, int b) {
    return a < b ? a : b;
}

static inline int max(int a, int b) {
    return a > b ? a : b;
}

static inline int32_t min32(int32_t a, int32_t b) {
    return a < b ? a : b;
}

static inline int32_t max32(int32_t a, int32_t b) {
    return a > b ? a : b;
}

static inline int64_t min64(int64_t a, int64_t b) {
    return a < b ? a : b;
}

static inline int64_t max64(int64_t a, int64_t b) {
    return a > b ? a : b;
}

static inline void setbit(uint8_t *bm, int idx) {
    bm[idx/8] |= (1 << (idx&7));
}

static inline bool getbit(uint8_t *bm, int idx) {
    return bm[idx/8] & (1 << (idx&7));
}

static inline char *bool2str(bool x) {
    return x ? "true" : "false";
}
#endif
#endif
