#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <sdlx.h>
#include <utils.h>

// buttons
#define MAX_BUTTON_ROW 7
#define MAX_BUTTON_COL 5

#define EVID_MODE   ('M' | 'O' << 8 | 'D' << 16 | 'E' << 24)
#define EVID_CLR    ('C' | 'L' << 8 | 'R' << 16)
#define EVID_CE     ('C' | 'E' << 8)
#define EVID_CHGSN  ('+' | '/' << 8 | '-' << 16)
#define EVID_SHL    ('<' | '<' << 8)
#define EVID_SHR    ('>' | '>' << 8)
#define BLANK 0

#define BUTTONS_X_LEFT     50
#define BUTTONS_X_SPACING 200
#define BUTTONS_Y_TOP     350
#define BUTTONS_Y_SPACING 200

int button[MAX_BUTTON_ROW][MAX_BUTTON_COL] = {
    { EVID_MODE, BLANK,  BLANK,   EVID_CE,      EVID_CLR,   },
    { '0',         '1',    '2',       '3',      EVID_CHGSN, },
    { '4',         '5',    '6',       '7',      '~',        },
    { '8',         '9',    'A',       'B',      BLANK,      },
    { 'C',         'D',    'E',       'F',      BLANK,      },
    { '&',         '|',    '^',       EVID_SHL, EVID_SHR,   },
    { '+',         '-',    '*',       '/',      '=',        },
        };

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
unsigned long process_op(int op, unsigned long operand1, unsigned long operand2);
void draw_button(int row, int col);

// -----------------  MAIN  ----------------------------------

int main(int argc, char **argv)
{
    int          rc;
    sdlx_event_t event;
    bool         done = false;

    int mode          = MODE_HEX;
    int display_state = RESULT;
    int op            = OP_NONE;
    unsigned long display_value = 0;
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
        sdlx_display_init(COLOR_BLACK);

        // register control event to
        // - end program
        sdlx_register_control_events(NULL, NULL, "X", COLOR_BLACK, 0, 0, EVID_QUIT);

        // register button events
        for (int row = 0; row < MAX_BUTTON_ROW; row++) {
            for (int col = 0; col < MAX_BUTTON_COL; col++) {
                draw_button(row, col);
            }
        }
        
        // display the most recent calculated value
        sdlx_render_printf(0, 0, "%20lX", display_value);
        //sdlx_render_printf(0, ROW2Y(1), "%20lX", operand_value);

        // present the display
        sdlx_display_present();

        // wait for event, with infinite timeout
        sdlx_get_event(-1, &event);

        // process event ...
        switch (event.event_id) {
        // control functions: clear, mode, and quit
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
            printf("got number button %d\n", n);
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

unsigned long process_op(int op, unsigned long operand1, unsigned long operand2)
{
    // xxx div by zero

    switch (op) {
    case '&': return operand1 & operand2;
    case '|': return operand1 | operand2;
    case '^': return operand1 ^ operand2;
    case EVID_SHL: return operand1 << operand2;
    case EVID_SHR: return operand1 >> operand2;
    case '+': return operand1 + operand2;
    case '-': return operand1 - operand2;
    case '*': return operand1 * operand2;
    case '/': return operand1 / operand2;
    }

    printf("ERROR %s: BUG process_op, op=%x\n", progname, op);
    return 0;
}

void draw_button(int row, int col)
{
    sdlx_loc_t *loc;
    int x, y;
    char str[8];

    if (button[row][col] == 0) {
        return;
    }    

    x = BUTTONS_X_LEFT + col * BUTTONS_X_SPACING;
    y = BUTTONS_Y_TOP + row * BUTTONS_Y_SPACING;

    memset(str, 0, sizeof(str));
    memcpy(str, &button[row][col], 4);
    loc = sdlx_render_printf_xyctr(x, y, "%s", str);
    sdlx_register_event(loc, button[row][col]);
}
