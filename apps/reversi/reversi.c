#include <common.h>

//
// notes:
// - black goes first
//

//
// defines
//

#define GAME_STATE_READY   1
#define GAME_STATE_ACTIVE  2
#define GAME_STATE_OVER    3

#define EVID_GAME_START   201
#define EVID_GAME_RESET   202
#define EVID_MOVE_PASS    203
#define EVID_END_PROGRAM  204

//
// typedefs
//

//
// variables
//

//
// prototypes
//

//xxx check these
static void game_init(board_t *b);
static bool humans_turn(board_t *b);
static void register_event(int evid);
static void apply_move(board_t *b, int move);
static bool is_game_over(board_t *b);
void get_possible_moves(board_t *b, possible_moves_t *pm);
static void draw_board(board_t *b, possible_moves_t *pm);

// -----------------  MAIN  ------------------------------------

int main(int argc, char **argv)
{
    bool             is_ez_app;
    int              game_state;
    board_t          board;
    possible_moves_t possible_moves;

    // init variables
    is_ez_app = (argc > 0 && strcmp(argv[0], "ez_app") == 0);
    game_state    = GAME_STATE_READY;
    game_init(&board);
    memset(&possible_moves, 0, sizeof(possible_moves));

    // if not ez_app then call sdl_init
    if (!is_ez_app && sdl_init() != 0) {
        printf("ERROR: sdl_init failed\n");
        return 1;
    }

    // loop until end program
    while (true) {
        // init display
        sdl_display_init(COLOR_BLACK);

        // display state
        // xxx todo

        // register for events based on game_state
        register_event(EVID_END_PROGRAM);
        if (game_state == GAME_STATE_READY) {
            register_event(EVID_GAME_START);
        } else if (game_state == GAME_STATE_ACTIVE) {
            register_event(EVID_GAME_RESET);
            if (humans_turn(&board)) {
                get_possible_moves(&board, &possible_moves);
                if (possible_moves.max == 0) {
                    register_event(EVID_MOVE_PASS);
                } else {
                    for (int i = 0; i < possible_moves.max; i++) {
                        register_event(possible_moves.move[i]);
                    }
                }
            }
        } else if (game_state == GAME_STATE_OVER) {
            register_event(EVID_GAME_RESET);
        }

        // draw the board
        draw_board(
            &board, 
            ((game_state == GAME_STATE_ACTIVE && humans_turn(&board)) 
             ? &possible_moves : NULL));

        // present display
        sdl_display_present();

        // if it is human's turn then 
        //   wait forever for an event
        // else if computer's turn then 
        //   poll for an event (no wait)
        // else 
        //   poll for an event (100 ms wait)
        // endif
        long        timeout;  // microsecs
        sdl_event_t event;
        if (game_state == GAME_STATE_ACTIVE && humans_turn(&board)) {
            timeout = -1;
        } else if (game_state == GAME_STATE_ACTIVE && !humans_turn(&board)) {
            timeout = 0;
        } else {
            timeout = 100000;  // 100 ms
        }
        sdl_get_event(timeout, &event);

        // process the event
        if (event.event_id == EVID_QUIT || event.event_id == EVID_END_PROGRAM) {
            break;
        } else if (event.event_id == EVID_GAME_RESET) {
            game_state = GAME_STATE_READY;
            game_init(&board);
        } else if (event.event_id == EVID_GAME_START) {
            game_state = GAME_STATE_ACTIVE;
        } else if (game_state == GAME_STATE_ACTIVE) {
            if (is_game_over(&board)) {
                game_state = GAME_STATE_OVER;
            } else if (humans_turn(&board)) {
                int move = (event.event_id == EVID_MOVE_PASS ? MOVE_PASS : event.event_id);
                apply_move(&board, move);
            } else {
                int move = cpu_get_move(&board);
                apply_move(&board, move);
            }
        } else if (game_state == GAME_STATE_OVER) {
            // nothing to do 
        }
    }

    // if not ez_app then call sdl_exit
    if (!is_ez_app) {
        sdl_exit();
    }

    // return success
    return 0;
}

// -----------------  SUPPORT  ------------------------------------

static void game_init(board_t *b)
{
    memset(b, 0, sizeof(board_t));

    b->pos[4][4]      = WHITE;
    b->pos[4][5]      = BLACK;
    b->pos[5][4]      = BLACK;
    b->pos[5][5]      = WHITE;
    b->black_cnt      = 2;
    b->white_cnt      = 2;
    b->whose_turn     = BLACK;
    b->black_is_human = true;
    b->white_is_human = false;
}

static bool humans_turn(board_t *b)
{
    return (b->whose_turn == BLACK && b->black_is_human) ||
           (b->whose_turn == WHITE && b->white_is_human);
}

static void register_event(int evid)
{
}

static void apply_move(board_t *b, int move)
{
}

static bool is_game_over(board_t *b)
{
    return true;
}

void get_possible_moves(board_t *b, possible_moves_t *pm)
{
}

static void draw_board(board_t *b, possible_moves_t *pm)
{
}

#if 0


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
        FATAL("pos[%d][%d] = %d\n", r, c, b->pos[r][c]); //xxx FATAL should be error
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
        FATAL("invalid call to apply_move, move=%d\n", move); //xxx
    }

    b->whose_turn = OTHER_COLOR(b->whose_turn);
}

void get_possible_moves(board_t *b, possible_moves_t *pm)
{
    int r, c, i, my_color, other_color, move;

    my_color = b->whose_turn;
    other_color = OTHER_COLOR(my_color);

    pm->max = 0;
    pm->color = my_color;  // xxx is this needed

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
}
#endif
