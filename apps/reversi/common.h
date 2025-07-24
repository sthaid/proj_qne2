#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <stdbool.h>

#include <sdl.h>
#include <utils.h>

#define NONE   0
#define BLACK  1
#define WHITE  2

#define MOVE_PASS -1

typedef struct {
    unsigned char pos[10][10];
    int black_cnt;
    int white_cnt;
    int whose_turn;
    bool black_is_human;
    bool white_is_human;
} board_t;

typedef struct {
    int move[64];
    int max;
} possible_moves_t;

int cpu_get_move(board_t *b);

#endif

