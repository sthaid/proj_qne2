#include <common.h>

//
// defines
//

#define GAME_STATE_READY   0
#define GAME_STATE_ACTIVE  1
#define GAME_STATE_OVER    2

#define EVID_END_PROG     1
#define EVID_GAME_START   2
#define EVID_GAME_RESET   3

//
// typedefs
//

typedef struct {
    int (*get_move)(board_t *b);
    char  name[100];
} player_t;

//
// prototypes
//

static void game_init(board_t *b);
static void apply_move(board_t *b, int move);
static bool is_game_over(board_t *b);
static void update_display(board_t *b);

// -----------------  MAIN  ------------------------------------

int main()
{
    int       w, h;
    int       game_state;
    player_t  *player_black;
    player_t  *player_white;
    board_t   board;

    int event_id, move;

    // init
#ifndef PICOC_VERSION
    sdl_init(&w, &h);  //xxx set w,h regarless
#endif
    game_state = GAME_STATE_READY;
    player_black = &(player_t){human_get_move, "HUMAN"};
    player_white = &(player_t){cpu_get_move, "CPU1"};
    memset(&board, 0, sizeof(board));

    // loop until end program
    while (true) {
        if (game_state == GAME_STATE_READY) {
            game_init(&board);
            update_display(&board);
            event_id = sdl_get_event(-1);
            if (event_id == EVID_GAME_START) {
                game_state = GAME_STATE_ACTIVE;
            }
            continue;
        }
            
        if (game_state == GAME_STATE_ACTIVE) {
            if (is_game_over(&board)) {
                game_state = GAME_STATE_OVER;
                continue;
            }

            player_t *player = (board.whose_turn == BLACK ? player_black : player_white);
            update_display(&board);
            move = player->get_move(&board);
            apply_move(&board, move);
            continue;
        }

        if (game_state == GAME_STATE_OVER) {
            update_display(&board);
            event_id = sdl_get_event(-1);
            if (event_id == EVID_GAME_RESET) {
                game_state = GAME_STATE_READY;
            }
            continue;
        }
    }

    sdl_exit();
    return 0;
}

// -----------------  SUPPORT  ------------------------------------

static void game_init(board_t *b)
{
    b->pos[4][4]  = WHITE;
    b->pos[4][5]  = BLACK;
    b->pos[5][4]  = BLACK;
    b->pos[5][5]  = WHITE;
    b->black_cnt  = 2;
    b->white_cnt  = 2;
    b->whose_turn = BLACK;
}

//                          0   1   2   3   4   5  6  7
static int r_incr_tbl[8] = {0, -1, -1, -1,  0,  1, 1, 1};
static int c_incr_tbl[8] = {1,  1,  0, -1, -1, -1, 0, 1};

void apply_move(board_t *b, int move)
{
    int  r, c, i, j, my_color, other_color;
    int *my_color_cnt, *other_color_cnt;
    bool succ;

    if (move == MOVE_PASS) {
        b->whose_turn = OTHER_COLOR(b->whose_turn);
        return;
    }

    my_color = b->whose_turn;
    other_color = OTHER_COLOR(my_color);

    succ = false;
    MOVE_TO_RC(move, r, c);
    if (b->pos[r][c] != NONE) {
        FATAL("pos[%d][%d] = %d\n", r, c, b->pos[r][c]);
    }

    if (my_color == BLACK) {
        my_color_cnt    = &b->black_cnt;
        other_color_cnt = &b->white_cnt;
    } else {
        my_color_cnt    = &b->white_cnt;
        other_color_cnt = &b->black_cnt;
    }

    b->pos[r][c] = my_color;
    *my_color_cnt += 1;

    for (i = 0; i < 8; i++) {
        int r_incr = r_incr_tbl[i];
        int c_incr = c_incr_tbl[i];
        int r_next = r + r_incr;
        int c_next = c + c_incr;
        int cnt    = 0;

        while (b->pos[r_next][c_next] == other_color) {
            r_next += r_incr;
            c_next += c_incr;
            cnt++;
        }

        if (cnt > 0 && b->pos[r_next][c_next] == my_color) {
            succ = true;
            r_next = r;
            c_next = c;
            for (j = 0; j < cnt; j++) {
                r_next += r_incr;
                c_next += c_incr;
                b->pos[r_next][c_next] = my_color;
            }
            *my_color_cnt += cnt;
            *other_color_cnt -= cnt;
        }
    }

    if (!succ) {
        FATAL("invalid call to apply_move, move=%d\n", move);
    }

    b->whose_turn = OTHER_COLOR(b->whose_turn);
}

void get_possible_moves(board_t *b, possible_moves_t *pm)
{
    int r, c, i, my_color, other_color, move;

    my_color = b->whose_turn;
    other_color = OTHER_COLOR(my_color);

    pm->max = 0;
    pm->color = my_color;

    for (r = 1; r <= 8; r++) {
        for (c = 1; c <= 8; c++) {
            if (b->pos[r][c] != NONE) {
                continue;
            }

            for (i = 0; i < 8; i++) {
                int r_incr = r_incr_tbl[i];
                int c_incr = c_incr_tbl[i];
                int r_next = r + r_incr;
                int c_next = c + c_incr;
                int cnt    = 0;

                while (b->pos[r_next][c_next] == other_color) {
                    r_next += r_incr;
                    c_next += c_incr;
                    cnt++;
                }

                if (cnt > 0 && b->pos[r_next][c_next] == my_color) {
                    RC_TO_MOVE(r, c, move);
                    pm->move[pm->max++] = move;
                    break;
                }
            }
        }
    }
}

static bool is_game_over(board_t *b)
{
    return false;
}

// -----------------  DISPLAY  ------------------------------------

sdl_create_filled_circle_texture
sdl_create_filled_circle_texture
sdl_render_fill_rect
sdl_font_char_height  and width

static void xxx(void)
{
        piece_circle_radius  = rint(.4*sq_wh);
        piece_black_circle   = sdl_create_filled_circle_texture(piece_circle_radius, SDL_BLACK);
        piece_white_circle   = sdl_create_filled_circle_texture(piece_circle_radius, SDL_WHITE);

        prompt_circle_radius = rint(.08*sq_wh);
        prompt_black_circle  = sdl_create_filled_circle_texture(prompt_circle_radius, SDL_BLACK);
        prompt_white_circle  = sdl_create_filled_circle_texture(prompt_circle_radius, SDL_WHITE);
}

static void update_display(board_t *b)
{
    sdl_display_init(COLOR_BLACK);

    sdl_display_present();
}

