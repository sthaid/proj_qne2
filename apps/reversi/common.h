#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <sdl.h>
#include <utils.h>

#define NONE   0
#define BLACK  1
#define WHITE  2

#define OTHER_COLOR(x) ((x) == BLACK ? WHITE : BLACK)

#define MOVE_PASS -1

#define HUMAN 0
#define CPU(level)  (level)

typedef struct {
    unsigned char pos[10][10];
    int black_cnt;
    int white_cnt;
    int whose_turn;
    int player_black;
    int player_white;
} board_t;

typedef struct {
    int move[64];
    int max;
} possible_moves_t;

int cpu_get_move(int level, board_t *b, char *eval_str);
void apply_move(board_t *b, int move);
void get_possible_moves(board_t *b, possible_moves_t *pm);
bool any_possible_moves(board_t *b);
void move_to_rc(int move, int *r, int *c);

#endif

