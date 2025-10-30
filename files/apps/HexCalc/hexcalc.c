#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <sdlx.h>
#include <utils.h>

// xxx
// - comments
// - cleanup

// buttons
#define MAX_BUTTON_ROW 7
#define MAX_BUTTON_COL 5

//#define EVID_MODE   ('M' | 'O' << 8 | 'D' << 16 | 'E' << 24)
#define EVID_MODE   ('M' | 'O' << 8 | 'D' << 16)
#define EVID_CLR    ('C' | 'L' << 8 | 'R' << 16)
#define EVID_CE     ('C' | 'E' << 8)
#define EVID_CHGSN  ('+' | '/' << 8 | '-' << 16)
#define EVID_SHL    ('<' | '<' << 8)
#define EVID_SHR    ('>' | '>' << 8)
#define BLANK 0

#define BUTTONS_X_LEFT    100
#define BUTTONS_X_SPACING 200
#define BUTTONS_Y_TOP     350
#define BUTTONS_Y_SPACING 200

//int button[MAX_BUTTON_ROW][MAX_BUTTON_COL] = {
//    { EVID_MODE, BLANK,  BLANK,   EVID_CE,      EVID_CLR,   },
//    { '0',         '1',    '2',       '3',      EVID_CHGSN, },  // xxx move 0 to end? and match other calculators
//    { '4',         '5',    '6',       '7',      '~',        },
//    { '8',         '9',    'A',       'B',      BLANK,      },
//    { 'C',         'D',    'E',       'F',      BLANK,      },
//    { '&',         '|',    '^',       EVID_SHL, EVID_SHR,   },
//    { '+',         '-',    '*',       '/',      '=',        },
//        };
int button[MAX_BUTTON_ROW][MAX_BUTTON_COL] = {
    { EVID_MODE, BLANK,  BLANK,   EVID_CE,      EVID_CLR,   },
    { 'C',         'D',    'E',       'F',      EVID_CHGSN, },  // xxx move 0 to end? and match other calculators
    { '8',         '9',    'A',       'B',      '~',        },
    { '4',         '5',    '6',       '7',      BLANK,      },
    { '0',         '1',    '2',       '3',      BLANK,      },
    { '&',         '|',    '^',       EVID_SHL, EVID_SHR,   },
    { '+',         '-',    '*',       '/',      '=',        },
        };

#define BACKGROUND_COLOR         COLOR_LIGHT_GRAY
#define BUTTON_COLOR_NORMAL      COLOR_WHITE
#define BUTTON_COLOR_HIGHLIGHT   COLOR_GRAY
#define BUTTON_TEXT_COLOR        COLOR_BLACK
#define HIGHLIGHT_DURATION_MS 100

// values for mode
#define MODE_HEX   0
#define MODE_DEC   1

// values for display_state
#define RESULT     0
#define NO_VALUE   1
#define INPUTTING  2

// values for op
#define OP_NONE 0

// variables
char *progname;
char *data_dir;

// prototypes
void evid_to_button_row_and_col(int evid, int *button_row, int *button_col);
unsigned long process_op(int op, unsigned long operand1, unsigned long operand2);
void draw_button(int row, int col, bool highlight);

// -----------------  MAIN  ----------------------------------

int main(int argc, char **argv)
{
    int          rc;
    sdlx_event_t event;
    bool         done = false;
    int          highlight_button_row = -1;
    int          highlight_button_col = -1;

    int mode          = MODE_HEX;  // xxx make global
    int display_state = RESULT;
    int op            = OP_NONE;
    unsigned long display_value = 0; // xxx make long
    unsigned long operand_value = 0;

    // get arg values
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s, wXh=%d %d\n",
           progname, data_dir, sdlx_win_width, sdlx_win_height);

    // init sdl video subsystem
    rc = sdlx_init(SUBSYS_VIDEO);
    if (rc != 0) {
        printf("ERROR %s: sdlx_init failed\n", progname);
        return 1;
    }

    // runtime loop
    while (!done) {
        // init the backbuffer
        sdlx_display_init(BACKGROUND_COLOR);

        // register control event to
        // - end program
        sdlx_register_control_events(NULL, NULL, "X", COLOR_BLACK, 0, 0, EVID_QUIT);  // xxx colors

        // register button events
        for (int row = 0; row < MAX_BUTTON_ROW; row++) {
            for (int col = 0; col < MAX_BUTTON_COL; col++) {
                bool highlight = (row == highlight_button_row && col == highlight_button_col);
                draw_button(row, col, highlight);
            }
        }
        
        // display the most recent calculated value
        // xxx make this a routine
        sdlx_render_printf(0, 0, "%20lX", display_value);
        //sdlx_render_printf(0, ROW2Y(1), "%20lX", operand_value);

        // present the display
        sdlx_display_present();

        // wait for event, with infinite timeout
        sdlx_get_event(highlight_button_row != -1 ? HIGHLIGHT_DURATION_MS * 1000 : -1, &event);

        // xxx comment
        if (event.event_id == -1) {
            highlight_button_row = -1;
            highlight_button_col = -1;
            continue;
        }

        evid_to_button_row_and_col(event.event_id, &highlight_button_row, &highlight_button_col);

        // process event ...
        switch (event.event_id) {
        // control functions: clear, clear entry, mode select, and quit
        case EVID_QUIT:
            done = true;
            break;
        case EVID_CLR:
            display_value = 0;
            display_state = RESULT;
            op = OP_NONE;
            operand_value = 0;
            break;
        case EVID_CE:
            if (display_state == INPUTTING) {
                display_value = 0;
            }
            break;
        case EVID_MODE:
            mode = (mode == MODE_HEX ? MODE_DEC : MODE_HEX);
            break;

        // number input events
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
        case 'A': case 'B': case 'C':  case 'D':  case 'E':  case 'F': {
            int n = (event.event_id <= '9' ? event.event_id - '0' : event.event_id - 'A' + 0xa);
            printf("INFO %s: got number button %d\n", progname, n);
            if (display_state == RESULT || display_state == NO_VALUE) {
                display_value = 0;
                display_state = INPUTTING;
            }
            display_value = (display_value << 4) | n;
            break; }

        // unary ops
        case EVID_CHGSN:
            display_value = -display_value;
            display_state = RESULT;
            break;
        case '~':
            display_value = ~display_value;
            display_state = RESULT;
            break;

        // binary ops
        case '&': case '|': case '^': case EVID_SHL: case EVID_SHR:
        case '+': case '-': case '*': case '/':
            if (display_state == NO_VALUE) {
                break;
            }
            if (op != OP_NONE) {
                display_value = process_op(op, operand_value, display_value);
            }
            operand_value = display_value;
            op = event.event_id;
            display_state = NO_VALUE;
            break;
        case '=':
            if (op != OP_NONE && display_state != NO_VALUE) {
                display_value = process_op(op, operand_value, display_value);
            }
            operand_value = 0;
            op = OP_NONE;
            display_state = RESULT;
        }
    }

    // cleanup and end program
    sdlx_quit(SUBSYS_VIDEO);
    printf("INFO %s: terminating\n", progname);
    return 0;
}

void evid_to_button_row_and_col(int evid, int *button_row, int *button_col)
{
    int row, col;

    for (row = 0; row < MAX_BUTTON_ROW; row++) {
        for (col = 0; col < MAX_BUTTON_COL; col++) {
            if (button[row][col] == evid) {
                *button_row = row;
                *button_col = col;
                return;
            }
        }
    }

    // xxx should never fail ?
    *button_row = -1;
    *button_col = -1;
}

unsigned long process_op(int op, unsigned long operand1, unsigned long operand2)
{
    // xxx div by zero

    switch (op) {
    case '&': return operand1 & operand2;
    case '|': return operand1 | operand2;
    case '^': return operand1 ^ operand2;
    case EVID_SHL: return operand1 << operand2;  // xxx cast to unsigned long
    case EVID_SHR: return operand1 >> operand2;
    case '+': return operand1 + operand2;
    case '-': return operand1 - operand2;
    case '*': return operand1 * operand2;
    case '/': return operand1 / operand2;
    }

    printf("ERROR %s: BUG process_op, op=%x\n", progname, op);
    return 0;
}

// xxx light grey bg with white button circles that highlight to dark gray
void draw_button(int row, int col, bool highlight)
{
    sdlx_loc_t loc;
    int x, y, radius;
    char str[8];

    static sdlx_texture_t *button_texture;
    static sdlx_texture_t *highlighted_button_texture;
    static int texture_w, texture_h;

    if (button_texture == NULL) {
        radius = BUTTONS_X_SPACING * 45 / 100;
        button_texture = sdlx_create_filled_circle_texture(radius, BUTTON_COLOR_NORMAL);  // xxx free
        highlighted_button_texture = sdlx_create_filled_circle_texture(radius,BUTTON_COLOR_HIGHLIGHT);
        sdlx_query_texture(button_texture, &texture_w, &texture_h);
        printf("texture w,h = %d %d\n", texture_w, texture_h);
    }

    if (button[row][col] == 0) {
        return;
    }    

    x = BUTTONS_X_LEFT + col * BUTTONS_X_SPACING;
    y = BUTTONS_Y_TOP + row * BUTTONS_Y_SPACING;

    // xxx use ?:
    if (!highlight) {
        sdlx_render_texture(x-texture_w/2, y-texture_h/2, texture_w, texture_h, button_texture);
    } else {
        sdlx_render_texture(x-texture_w/2, y-texture_h/2, texture_w, texture_h, highlighted_button_texture);
    }

    sdlx_print_init(20, BUTTON_TEXT_COLOR, !highlight ? BUTTON_COLOR_NORMAL : BUTTON_COLOR_HIGHLIGHT);
    memset(str, 0, sizeof(str));
    memcpy(str, &button[row][col], 4);
    sdlx_render_printf_xyctr(x, y, "%s", str);

    loc.x = x - texture_w/2;
    loc.y = y - texture_h/2;
    loc.w = texture_w;
    loc.h = texture_h;
    sdlx_register_event(&loc, button[row][col]);
}
