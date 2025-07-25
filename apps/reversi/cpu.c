// xxx rename file

#include <common.h>

//
// defines
//

#define INFIN 0x7fffffffffffffff
#define ONE64 ((long)1)

//
// variables
//

//
// prototypes
//

static long alphabeta(board_t *b, int depth, long alpha, long beta, bool maximizing_player, int *move);
static void init_edge_gateway_to_corner(void);
static long heuristic(board_t *b, bool maximizing_player, bool game_over, possible_moves_t *pm);

static long min64(long a, long b) {
    return a < b ? a : b;
}

static long max64(long a, long b) {
    return a > b ? a : b;
}

#if 0
static void setbit(unsigned char *bm, int idx) {
    bm[idx/8] |= (1 << (idx&7));
}

static bool getbit(unsigned char *bm, int idx) {
    return bm[idx/8] & (1 << (idx&7));
}
#endif

#if 0
static void move_to_rc(int move, int *r, int *c)
{
    *r = move / 10;
    *c = move % 10;
}
#endif


// -----------------  CPU PLAYER - GET_MOVE ---------------------------------

static int  MIN_DEPTH[9]              = {0,  1,  2,  3,  4,  5,  6,  7,  8 };
static int  PIECECNT_FOR_EOG_DEPTH[9] = {0, 56, 55, 54, 53, 52, 51, 50, 49 };
static bool initialized               = false;

static int get_depth(int level, int piececnt)
{
    double M, B;
    int depth;

    M = 1.0;
    B = (64 - PIECECNT_FOR_EOG_DEPTH[level]) - M * PIECECNT_FOR_EOG_DEPTH[level];
    depth = rint(M * piececnt + B);
    if (depth < MIN_DEPTH[level]) depth = MIN_DEPTH[level];
    return depth;
}

static void create_eval_str(long value, char *eval_str);

int cpu_get_move(int level, board_t *b, char *eval_str)
{
    long value;
    int     move, depth, piececnt;

    // sanity check level arg
    if (level < 1 || level > 8) {
        // xxx
        printf("ERROR: invlaid level %d\n", level);
        return MOVE_PASS;
    }

    // initialization
    if (initialized == false) {
        init_edge_gateway_to_corner();
        initialized = true;
    }
    piececnt = b->black_cnt + b->white_cnt;

    // get lookahead depth
    depth = get_depth(level, piececnt);
    printf("depth = %d\n",depth);

    // call alphabeta to get the best move, and associated heuristic value
    value = alphabeta(b, depth, -INFIN, +INFIN, true, &move);

    // create evaluation string, based on value returned from alphabeta;
    // this string can optionally be displayed by caller
    create_eval_str(value, eval_str);

    // return the move, that was obtained by call to alphabeta
    return move;
}

static void create_eval_str(long value, char *eval_str)
{
    // eval_str should not exceed 16 char length, 
    // to avoid characters being off the window

    if (eval_str == NULL) {
        return;
    }

    if (value == (ONE64 << 56)) {
        sprintf(eval_str, "TIE");
    } else if (value > (ONE64 << 56)) {
        sprintf(eval_str, "CPU TO WIN BY %d", (int)((value >> 56) - 1));
    } else if (value < -(ONE64 << 56)) {
        sprintf(eval_str, "HUMAN CAN WIN BY %d", (int)(-(value >> 56) - 1));
    } else {
        eval_str[0] = '\0';
    }
}

// -----------------  CHOOSE BEST MOVE (RECURSIVE ROUTINE)  -----------------

static board_t *CHILD(board_t *b, board_t *b_child, int move)
{
    *b_child = *b;
    apply_move(b_child, move);
    return b_child;
}

static long alphabeta(board_t *b, int depth, long alpha, long beta, bool maximizing_player, int *move)
{
    long          value, v;
    int              i, best_move = MOVE_PASS;
    board_t          b_child;
    bool             game_over;
    possible_moves_t pm;

    // determine values for game_over and pm (possible moves)
    //
    // if all squares are filled then
    //   the game is over and there are no possible moves
    // else
    //   Call get_possible_moves to obtain a list of the possible moves.
    //   If there are no possible moves this means that either the game is
    //    over or the player must pass. The determination of which is made by
    //    checking if the opponent has possible moves. If the oppenent has no
    //    possible moves then the game is over. If the opponent has possible
    //    moves then this player must pass.
    // endif
    // xxx optimize
    // XXX
    game_over = false;
    if (b->black_cnt + b->white_cnt == 64) {
        game_over = true;
        pm.max = 0;
    } else {
        get_possible_moves(b, &pm);
        if (pm.max == 0) {
#if 0 //xxx
            possible_moves_t other_pm;
            get_possible_moves(CHILD(MOVE_PASS), &other_pm);
            game_over = (other_pm.max == 0);
            if (!game_over) {
                pm.max = 1;
                pm.move[0] = MOVE_PASS;
            }
#endif
        }
    }

    // 
    // The following is the alpha-beta pruning algorithm, ported from
    //  https://en.wikipedia.org/wiki/Alpha%E2%80%93beta_pruning
    // The algorithm has been updated to return the best_move along with the value.
    // 

    if (depth == 0 || game_over) {
        if (move) *move = MOVE_PASS;   // xxx or MOVE_NONE?
        return heuristic(b, maximizing_player, game_over, &pm);
    }

    if (maximizing_player) {
        value = -INFIN;
        for (i = 0; i < pm.max; i++) {
            if ((v = alphabeta(CHILD(b, &b_child, pm.move[i]), depth-1, alpha, beta, false, NULL)) > value) {
                value = v;
                best_move = pm.move[i];
            }
            alpha = max64(alpha, value);
            if (alpha >= beta) {
                break;
            }
        }
    } else {
        value = +INFIN;
        for (i = 0; i < pm.max; i++) {
            if ((v = alphabeta(CHILD(b, &b_child, pm.move[i]), depth-1, alpha, beta, true, NULL)) < value) {
                value = v;
                best_move = pm.move[i];
            }
            beta = min64(beta, value);
            if (beta <= alpha) {
                break;
            }
        }
    }

    if (move) *move = best_move;
    return value;
}

// -----------------  HEURISTIC  ---------------------------------------------------

static long corner_count(board_t *b)
{
    int cnt = 0;
    int my_color = b->whose_turn;
    int other_color = OTHER_COLOR(my_color);

    if (b->pos[1][1] == my_color) cnt++;
    if (b->pos[1][1] == other_color) cnt--;
    if (b->pos[1][8] == my_color) cnt++;
    if (b->pos[1][8] == other_color) cnt--;
    if (b->pos[8][1] == my_color) cnt++;
    if (b->pos[8][1] == other_color) cnt--;
    if (b->pos[8][8] == my_color) cnt++;
    if (b->pos[8][8] == other_color) cnt--;
    return cnt;
}

#if 0 //xxx
    static struct {
        int r;
        int c;
        int r_incr_tbl[3];
        int c_incr_tbl[3];
    } tbl[4] = {         // which_corner: 
        { 1,1, {0, 1, 1}, { 1, 0, 1} },    // 0: top left
        { 1,8, {0, 1, 1}, {-1, 0,-1} },    // 1: top right
        { 8,8, {0,-1,-1}, {-1, 0,-1} },    // 2: bottom right
        { 8,1, {0,-1,-1}, { 1, 0, 1} },    // 3: bottom left
                    };

static bool is_corner_move_possible(board_t *b, int which_corner)
{
    int r, c, i, my_color, other_color;

    r = tbl[which_corner].r;
    c = tbl[which_corner].c;
    if (b->pos[r][c] != NONE) {
        return false;
    }

    my_color    = b->whose_turn;
    other_color = OTHER_COLOR(my_color);
    for (i = 0; i < 3; i++) {
        int r_incr = tbl[which_corner].r_incr_tbl[i];
        int c_incr = tbl[which_corner].c_incr_tbl[i];
        int r_next = r + r_incr;
        int c_next = c + c_incr;
        int cnt    = 0;

        while (b->pos[r_next][c_next] == other_color) {
            r_next += r_incr;
            c_next += c_incr;
            cnt++;
        }

        if (cnt > 0 && b->pos[r_next][c_next] == my_color) {
            return true;
        }
    }

    return false;
}
#else
static bool is_corner_move_possible(board_t *b, int which_corner)
{
    return false;
}
#endif

static long corner_moves(board_t *b)
{
    int cnt = 0;
    int my_color = b->whose_turn;
    int other_color = OTHER_COLOR(my_color);
    int which_corner;

    for (which_corner = 0; which_corner < 4; which_corner++) {
        if (is_corner_move_possible(b, which_corner)) cnt++;
    }

    ((board_t*)b)->whose_turn = other_color;
    for (which_corner = 0; which_corner < 4; which_corner++) {
        if (is_corner_move_possible(b, which_corner)) cnt--;
    }
    ((board_t*)b)->whose_turn = my_color;

    return cnt;
}

static long diagonal_gateways_to_corner(board_t *b)
{
    int cnt = 0;
    int my_color = b->whose_turn;
    int other_color = OTHER_COLOR(my_color);

    if (b->pos[1][1] == NONE) {
        if (b->pos[2][2] == other_color) cnt++;
        if (b->pos[2][2] == my_color) cnt--;
    }
    if (b->pos[1][8] == NONE) {
        if (b->pos[2][7] == other_color) cnt++;
        if (b->pos[2][7] == my_color) cnt--;
    }
    if (b->pos[8][1] == NONE) {
        if (b->pos[7][2] == other_color) cnt++;
        if (b->pos[7][2] == my_color) cnt--;
    }
    if (b->pos[8][8] == NONE) {
        if (b->pos[7][7] == other_color) cnt++;
        if (b->pos[7][7] == my_color) cnt--;
    }

    return cnt;
}

//static unsigned char black_gateway_to_corner_bitmap[8192];
//static unsigned char white_gateway_to_corner_bitmap[8192];

static long edge_gateway_to_corner(board_t *b)
{
return 0;
#if 0 //xxx
    #define HORIZONTAL_EDGE(b, r) \
        ((b->pos[r][1] << 14) | \
         (b->pos[r][2] << 12) | \
         (b->pos[r][3] << 10) | \
         (b->pos[r][4] <<  8) | \
         (b->pos[r][5] <<  6) | \
         (b->pos[r][6] <<  4) | \
         (b->pos[r][7] <<  2) | \
         (b->pos[r][8] <<  0))
    #define VERTICAL_EDGE(b, c) \
        ((b->pos[1][c] << 14) | \
         (b->pos[2][c] << 12) | \
         (b->pos[3][c] << 10) | \
         (b->pos[4][c] <<  8) | \
         (b->pos[5][c] <<  6) | \
         (b->pos[6][c] <<  4) | \
         (b->pos[7][c] <<  2) | \
         (b->pos[8][c] <<  0))

    int       my_color    = b->whose_turn;
    int       other_color = OTHER_COLOR(my_color);
    int       cnt         = 0;
    unsigned short  edge;
    unsigned char  *my_color_gateway_to_corner_bitmap;
    unsigned char  *other_color_gateway_to_corner_bitmap;

    my_color_gateway_to_corner_bitmap = (my_color == BLACK 
        ? black_gateway_to_corner_bitmap : white_gateway_to_corner_bitmap);
    other_color_gateway_to_corner_bitmap = (other_color == BLACK 
        ? black_gateway_to_corner_bitmap : white_gateway_to_corner_bitmap);

    edge = HORIZONTAL_EDGE(b,1);
    if (getbit(my_color_gateway_to_corner_bitmap,edge)) cnt++;
    if (getbit(other_color_gateway_to_corner_bitmap,edge)) cnt--;

    edge = HORIZONTAL_EDGE(b,8);
    if (getbit(my_color_gateway_to_corner_bitmap,edge)) cnt++;
    if (getbit(other_color_gateway_to_corner_bitmap,edge)) cnt--;

    edge = VERTICAL_EDGE(b,1);
    if (getbit(my_color_gateway_to_corner_bitmap,edge)) cnt++;
    if (getbit(other_color_gateway_to_corner_bitmap,edge)) cnt--;

    edge = VERTICAL_EDGE(b,8);
    if (getbit(my_color_gateway_to_corner_bitmap,edge)) cnt++;
    if (getbit(other_color_gateway_to_corner_bitmap,edge)) cnt--;

    return cnt;
#endif
}

static void init_edge_gateway_to_corner(void)
{
#if 0 //xxx
    #define REVERSE(x) \
        ((((x) & 0x0003) << 14) | \
         (((x) & 0x000c) << 10) | \
         (((x) & 0x0030) <<  6) | \
         (((x) & 0x00c0) <<  2) | \
         (((x) & 0x0300) >>  2) | \
         (((x) & 0x0c00) >>  6) | \
         (((x) & 0x3000) >> 10) | \
         (((x) & 0xc000) >> 14))


    #define MAX_BLACK_GATEWAY_TO_CORNER_PATTERNS (sizeof(black_gateway_to_corner_patterns)/sizeof(char*))

    static char *black_gateway_to_corner_patterns[] = {
                ".W.W....",
                ".W.WW...",
                ".W.WWW..",
                ".W.WWWW.",
                ".WW.W...",
                ".WW.WW..",
                ".WW.WWW.",
                ".WWW.W..",
                ".WWW.WW.",
                ".W...W..",
                ".W...WW.",
                ".W..B...",
                ".WW..B..",
                ".W.B.B..",
                            };

    int i,j;
    unsigned short edge, edge_reversed;

    for (i = 0; i < MAX_BLACK_GATEWAY_TO_CORNER_PATTERNS; i++) {
        char *s = black_gateway_to_corner_patterns[i];
        edge = 0;
        for (j = 0; j < 8; j++) {
            if (s[j] == 'W') {
                edge |= (WHITE << (14-2*j));
            } else if (s[j] == 'B') {
                edge |= (BLACK << (14-2*j));
            }
        }
        edge_reversed = REVERSE(edge);
        //printf("BLACK PATTERN  %04x  %04x\n", edge, edge_reversed);
        setbit(black_gateway_to_corner_bitmap, edge);
        setbit(black_gateway_to_corner_bitmap, edge_reversed);
    }

    for (i = 0; i < MAX_BLACK_GATEWAY_TO_CORNER_PATTERNS; i++) {
        char *s = black_gateway_to_corner_patterns[i];
        edge = 0;
        for (j = 0; j < 8; j++) {
            if (s[j] == 'W') {
                edge |= (BLACK << (14-2*j));
            } else if (s[j] == 'B') {
                edge |= (WHITE << (14-2*j));
            }
        }
        edge_reversed = REVERSE(edge);
        //printf("WHITE PATTERN  %04x  %04x\n", edge, edge_reversed);
        setbit(white_gateway_to_corner_bitmap, edge);
        setbit(white_gateway_to_corner_bitmap, edge_reversed);
    }
#endif
}

static long reasonable_moves(board_t *b, possible_moves_t *pm)
{
    int i, cnt = pm->max;

    for (i = 0; i < pm->max; i++) {
        int r,c;
        move_to_rc(pm->move[i], &r, &c);
        if ((r == 2 && c == 2 && b->pos[1][1] == NONE) ||
            (r == 2 && c == 7 && b->pos[1][8] == NONE) ||
            (r == 7 && c == 2 && b->pos[8][1] == NONE) ||
            (r == 7 && c == 7 && b->pos[8][8] == NONE))
        {
            cnt--;
        }
    }

    return cnt;
}

// - - - - - - - - - - - - - - - - - - 

static long heuristic(board_t *b, bool maximizing_player, bool game_over, possible_moves_t *pm)
{
    long value;

    // handle game over case
    if (game_over) {
        long piece_cnt_diff;

        // the game is over, return a large positive or negative value, 
        // incorporating by how many pieces the game has been won or lost
        piece_cnt_diff = (b->whose_turn == BLACK ? b->black_cnt - b->white_cnt
                                                 : b->white_cnt - b->black_cnt);
        if (piece_cnt_diff >= 0) {
            value = (piece_cnt_diff+1) << 56;
        } else {
            value = (piece_cnt_diff-1) << 56;
        }

        // the returned heuristic value measures the favorability 
        // for the maximizing player
        return (maximizing_player ? value : -value);
    }

    // game is not over ...

    // The following board characteristics are utilized to generate the heuristic value.
    //
    // These are listed in order of importance / numeric weight.
    //
    // Except as noted, each characteristic is evaluated as a poistive for the color 
    // whose turn it is, and a negative for the other color. For example, if it is
    // black's turn and black has pieces in 2 corners and white has pieces in 1 corner
    // then the corner_count would be 1.
    //
    // - corner_count: count of occupied corners
    //
    // - corner_moves: number of corners that can be captured
    //
    // - diagonal_gateways_to_corner: number of pieces that are diagonally inside
    //   an unoccupied corner; this count is negative for my_color and positive for other_color
    //
    // - edge_gateways_to_corner: certain edge patterns can lead to future corner 
    //   capture; this counts the number of occurances of an edge pattern that can
    //   lead to future corner capture
    //
    // - reasonable_moves: this characteristic is evaluated only for the color whose turn it
    //   is; this is the count of possible moves minus possible moves that are not
    //   reasonable; the moves that are not considered reasonable are moves that provide
    //   a diagonal gateway to a corner
    //
    // - random value so that the same move is not always chosen; this covers the
    //   case where heuristic value calculated from the above characteristics are the
    //   same for differnent board inputs

    value = 0;
    value += (corner_count(b) << 48);
    value += (corner_moves(b) << 40);
    value += (diagonal_gateways_to_corner(b) << 32);
    value += (edge_gateway_to_corner(b) << 24);
    value += (reasonable_moves(b, pm) << 16);
    value += ((random() & 127) - 64);

    // the returned heuristic value measures the favorability 
    // for the maximizing player
    return (maximizing_player ? value : -value);
}
