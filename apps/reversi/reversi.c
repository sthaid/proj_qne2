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
#define GAME_STATE_ERROR   4

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

static void draw_init(void);
static void draw_unload(void);
static void draw_board(board_t *b, possible_moves_t *pm);

//xxx check these
static void game_init(board_t *b);
static bool humans_turn(board_t *b);
static void register_event(int evid);
static int apply_move(board_t *b, int move);
void get_possible_moves(board_t *b, possible_moves_t *pm);
static bool any_possible_moves(board_t *b);
static bool is_game_over(board_t *b);


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

    // init print; 20 chars across display
    // xxx check that is the default, for both ez_app and !ex_app modes
    // xxx and maybe remove from here
    sdl_print_init(20, COLOR_WHITE, COLOR_BLACK);

    draw_init();

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
        } else if (game_state == GAME_STATE_ERROR) {
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

        // if no event then continue
        if (event.event_id == -1) {
            continue;
        }

        // process the event
        printf("GOT EVENT %d\n", event.event_id);
        if (event.event_id == EVID_QUIT || event.event_id == EVID_END_PROGRAM) {
            break;
        } else if (event.event_id == EVID_GAME_RESET) {
            game_state = GAME_STATE_READY;
            game_init(&board);
        } else if (event.event_id == EVID_GAME_START) { //xxx fix
            game_state = GAME_STATE_ACTIVE;
        } else if (game_state == GAME_STATE_ACTIVE) {
            int rc = 0;
            if (is_game_over(&board)) {
                game_state = GAME_STATE_OVER;
            } else if (humans_turn(&board)) {
                int move = (event.event_id == EVID_MOVE_PASS ? MOVE_PASS : event.event_id);
                rc = apply_move(&board, move);
            } else {
                int move = cpu_get_move(&board);
                rc = apply_move(&board, move);
            }
            if (rc != 0) {
                game_state = GAME_STATE_ERROR;
            }
        } else if (game_state == GAME_STATE_OVER) {
            //  xxx comment
        } else if (game_state == GAME_STATE_ERROR) {
            //  xxx comment
        }
    }

    // xxx
    draw_unload();

    // if not ez_app then call sdl_exit
    if (!is_ez_app) {
        sdl_exit();
    }

    // return success
    return 0;
}

// -----------------  DRAW BOARD  ---------------------------------

// xxx rename xywh
static void rc_to_loc(int r_arg, int c_arg, int *x, int *y, int *w, int *h);

static struct { //xxx
    int x;
    int y;
    int w;
    int h;
} loc[10][10];  // xxx picoc?

static int            piece_circle_radius;
static sdl_texture_t *piece_black_circle;
static sdl_texture_t *piece_white_circle;

static int            prompt_circle_radius;
static sdl_texture_t *prompt_black_circle;
static sdl_texture_t *prompt_white_circle;

//static int            status_circle_radius;
//static sdl_texture_t *status_black_circle;
//static sdl_texture_t *status_white_circle;

//double rint(double x) {  //xxx
//    return x+.5;
//}

static void draw_init(void)
{
    int r,c;

    for (r = 1; r <= 8; r++) {
        for (c = 1; c <= 8; c++) {
            loc[r][c].x = 1 + 125 * (c - 1);
            loc[r][c].y = 1 + 125 * (r - 1);
            loc[r][c].w = 124;
            loc[r][c].h = 124;
        }
    }


    int sq_wh = 123;

    piece_circle_radius  = rint(.4*sq_wh);   // xxx need rint
    piece_black_circle   = sdl_create_filled_circle_texture(piece_circle_radius, COLOR_BLACK);
    piece_white_circle   = sdl_create_filled_circle_texture(piece_circle_radius, COLOR_WHITE);

    prompt_circle_radius = rint(.08*sq_wh);
    prompt_black_circle  = sdl_create_filled_circle_texture(prompt_circle_radius, COLOR_BLACK);
    prompt_white_circle  = sdl_create_filled_circle_texture(prompt_circle_radius, COLOR_WHITE);

    //status_circle_radius = rint(.36*FONTSZ);
    //status_black_circle  = sdl_create_filled_circle_texture(status_circle_radius, COLOR_BLACK);
    //status_white_circle  = sdl_create_filled_circle_texture(status_circle_radius, COLOR_WHITE);
}

static void draw_unload(void)
{
    sdl_destroy_texture(piece_black_circle);
    sdl_destroy_texture(piece_white_circle);

    sdl_destroy_texture(prompt_black_circle);
    sdl_destroy_texture(prompt_white_circle);
}

static void rc_to_loc(int r_arg, int c_arg, int *x, int *y, int *w, int *h)
{
    static bool first_call = true;

    if (first_call) {
        first_call = false;
    }

    *x = loc[r_arg][c_arg].x;
    *y = loc[r_arg][c_arg].y;
    *w = loc[r_arg][c_arg].w;
    *h = loc[r_arg][c_arg].h;
}

static void draw_board(board_t *b, possible_moves_t *pm)
{
    int x1, x2, y1, y2;
    int i;

    //sdl_render_fill_rect(0, 0, 1000, 1000, COLOR_GREEN);
    sdl_render_fill_rect(1, 1, 998, 998, COLOR_GREEN);
    for (i = 0; i < 9; i++) {
        x1 = x2 = 125 * i;
        y1 = 0;
        y2 = 999;
        sdl_render_line(x1, y1, x2, y2, COLOR_BLACK);

        x1 = 0;
        x2 = 999;
        y1 = y2 = 125 * i;
        sdl_render_line(x1, y1, x2, y2, COLOR_BLACK);
    }

    // draw the black and white pieces 
    int r, c, x, y, w, h, offset;
    sdl_texture_t *piece;

    for (r = 1; r <= 8; r++) {
        for (c = 1; c <= 8; c++) {
            if (b->pos[r][c] != NONE) {
                piece = (b->pos[r][c] == BLACK ? piece_black_circle : piece_white_circle);
                rc_to_loc(r, c, &x, &y, &w, &h);
                offset = w / 2 - piece_circle_radius;
                sdl_render_texture(x+offset, y+offset, -1, -1, 0, piece);
            }
        }
    }
}


// -----------------  SUPPORT ROUTINES  ---------------------------

static int r_incr_tbl[8] = {0, -1, -1, -1,  0,  1, 1, 1};  //xxx does this work in picoc?
static int c_incr_tbl[8] = {1,  1,  0, -1, -1, -1, 0, 1};

static void move_to_rc(int move, int *r, int *c)
{
    *r = move / 10;
    *c = move % 10;
}

static int rc_to_move(int r, int c)
{
    return r * 10 + c;
}

static bool humans_turn(board_t *b)
{
    return (b->whose_turn == BLACK && b->black_is_human) ||
           (b->whose_turn == WHITE && b->white_is_human);
}

#define NK2X(n,k) ((sdl_win_width/2/(n)) + (k) * (sdl_win_width/(n)))

static void register_event(int evid)
{
    sdl_loc_t *loc;
    sdl_loc_t loc2;
    int move, r, c, x, y, w, h;

    switch (evid) {
    case EVID_GAME_START:
        loc = sdl_render_text(0, 1100, "START");
        break;
    case EVID_GAME_RESET:
        loc = sdl_render_text(0, 1100, "RESET");
        break;
    case EVID_MOVE_PASS:
        loc = sdl_render_text(0, 1200, "PASS");
        break;
    case EVID_END_PROGRAM:
        sdl_print_init(10, COLOR_WHITE, COLOR_BLACK);
        loc = sdl_render_printf_xyctr(NK2X(3,2), sdl_win_height-sdl_char_height/2, "%s", "X");
        sdl_print_init(20, COLOR_WHITE, COLOR_BLACK);
        break;
    default:
        move = evid;
        move_to_rc(move, &r, &c);
        rc_to_loc(r, c, &x, &y, &w, &h);
        loc2.x = x;
        loc2.y = y;
        loc2.w = w;
        loc2.h = h;
        loc = &loc2;
        printf("REGISTER MOVE %d %d %d %d\n", x, y, w, h);
        break;
    }

    sdl_register_event(loc, evid);
}

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

static int apply_move(board_t *b, int move)
{
    int  r, c, i, j, my_color, other_color;
    int *my_color_cnt, *other_color_cnt;
    bool succ;

    printf("apply_move called: move=%d color=%d\n", move, b->whose_turn);

    if (move == MOVE_PASS) {
        b->whose_turn = OTHER_COLOR(b->whose_turn);
        return 0;
    }

    my_color = b->whose_turn;
    other_color = OTHER_COLOR(my_color);

    succ = false;
    move_to_rc(move, &r, &c);
    if (b->pos[r][c] != NONE) {
        printf("ERROR: pos[%d][%d] is occupied, color=%d\n", r, c, b->pos[r][c]);
        return -1;
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
        printf("ERROR: invalid call to apply_move, move=%d\n", move);
        return -1;
    }

    b->whose_turn = OTHER_COLOR(b->whose_turn);
    return 0;
}

void get_possible_moves(board_t *b, possible_moves_t *pm)
{
    int r, c, i, my_color, other_color;

    my_color = b->whose_turn;
    other_color = OTHER_COLOR(my_color);

    pm->max = 0;

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
                    pm->move[pm->max++] = rc_to_move(r, c);
                    break;
                }
            }
        }
    }
}

static bool any_possible_moves(board_t *b)
{
    int r, c, i, my_color, other_color;

    my_color = b->whose_turn;
    other_color = OTHER_COLOR(my_color);

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
                    return true;
                }
            }
        }
    }

    return false;
}

static bool is_game_over(board_t *b)
{
    bool apm;

    // quick check, if all squares are filled the game is over
    if (b->black_cnt + b->white_cnt == 64) {
        return true;
    }

    // if the current player has possible moves then the game is not over
    if (any_possible_moves(b)) {
        return false;
    }

    // if the other player does not have possible moves the game is over
    b->whose_turn = OTHER_COLOR(b->whose_turn);
    apm = any_possible_moves(b);
    b->whose_turn = OTHER_COLOR(b->whose_turn);
    return !apm;
}
