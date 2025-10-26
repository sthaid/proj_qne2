#include <stdio.h>
#include <stdbool.h>

#include <sdlx.h>
#include <utils.h>

// defines
#define EVID_0 10
#define EVID_BACKSPACE    30
#define EVID_CHANGE_SIGN  32
#define EVID_CLEAR        33
#define EVID_EQUALS       35

#define EVID_PLUS        36
#define EVID_MINUS       37
#define EVID_TIMES       38
#define EVID_DIVIDE      39

#define EVID_NOT          40
#define EVID_AND          41
#define EVID_OR           42
#define EVID_XOR          43
#define EVID_LEFT_SHIFT   44
#define EVID_RIGHT_SHIFT  45

#define EVID_MODE 46
#define EVID_BITS 47

#define MAX_BUTTON_ROW 7
#define MAX_BUTTON_COL 5

#define BUTTONS_X_LEFT     50
#define BUTTONS_X_SPACING 200

#define BUTTONS_Y_TOP     350
#define BUTTONS_Y_SPACING 200

#define RESULT 1
#define INPUT_VALUE  2

#define MAX_MODE  2
#define MODE_HEX   0
#define MODE_DECIMAL 1



// typedefs
typedef struct {
    int evid;
    char *str;
} button_t;

// variables
char    *progname;
char    *data_dir;
button_t button[MAX_BUTTON_ROW][MAX_BUTTON_COL];
int      mode = MODE_HEX;
int      bits = 64;

// prototypes
void init_buttons(void);
void draw_button(int row, int col);

// -----------------  MAIN  ------------------------------------------

int   displaying = INPUT_VALUE;
unsigned long  display_value;
    
unsigned long operand;
int operation;

#define OP_NONE  0
#define OP_AND   1
#define OP_PLUS  2
#define OP_MINUS 3
#define OP_TIMES 4
#define OP_DIVIDE 5

int main(int argc, char **argv)
{
    int          rc;
    sdlx_event_t event;
    bool         done = false;

    // get arg values
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s, wXh=%d %d\n",
           progname, data_dir, sdlx_win_width, sdlx_win_height);

    // xxx
    init_buttons();

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

        // xxx comment
        button[6][1].str = (mode == MODE_HEX ? "HEX" : "DEC");
        button[6][2].str = (bits == 32 ? "32" : "64");

        // register button events
        for (int row = 0; row < MAX_BUTTON_ROW; row++) {
            for (int col = 0; col < MAX_BUTTON_COL; col++) {
                draw_button(row, col);
            }
        }
        
        // display value xxx account for bits ?
        if (mode == MODE_HEX) {
            sdlx_render_printf(0, 0, "%20lX", display_value);
        } else {
            sdlx_render_printf(0, 0, "%20ld", display_value);  // xxx needs work for max values
        }

        // present the display
        sdlx_display_present();

        // wait for event, with infinite timeout
        sdlx_get_event(-1, &event);

        // process control functions: clear, mode, and quit
        if (event.event_id == EVID_CLEAR) {
            display_value = 0;
            displaying = RESULT;
        } else if (event.event_id == EVID_MODE) {
            mode = (mode + 1) % MAX_MODE;
        } else if (event.event_id == EVID_BITS) {
            bits = (bits == 32 ? 64 : 32);
            //if (bits == 32) {
            //    display_value32 = display_value64;
            //}
        } else if (event.event_id == EVID_QUIT) {
            done = true;

        // process number input events
        } else if (event.event_id >= EVID_0 && event.event_id < EVID_0 + 16) {
            int n = event.event_id - EVID_0;
            printf("got number button %d\n", n);
            if (displaying == RESULT) {
                display_value = 0;
                displaying = INPUT_VALUE;
            }
            display_value = (display_value << 4) | n;
        } else if (event.event_id == EVID_BACKSPACE) {
            display_value = (unsigned long)display_value >> 4;

        // process unary ops
        } else if (event.event_id == EVID_NOT) {
            display_value = ~display_value;
            displaying = RESULT;
        } else if (event.event_id == EVID_CHANGE_SIGN) {
            display_value = -display_value;
        } else {
            // process binary ops
            switch (event.event_id) {
            case EVID_AND:
                operand = display_value;
                operation = OP_AND;
                displaying = RESULT;
                break;
            case EVID_PLUS:
                operand = display_value;
                operation = OP_PLUS;
                displaying = RESULT;
                break;
            case EVID_MINUS:
                operand = display_value;
                operation = OP_MINUS;
                displaying = RESULT;
                break;
            case EVID_TIMES:
                operand = display_value;
                operation = OP_TIMES;
                displaying = RESULT;
                break;
            case EVID_DIVIDE:
                operand = display_value;
                operation = OP_DIVIDE;
                displaying = RESULT;
                break;
            case EVID_OR:
                break;
            case EVID_XOR:
                break;
            case EVID_LEFT_SHIFT:
                break;
            case EVID_RIGHT_SHIFT:
                break;
            case EVID_EQUALS:
                switch (operation) {
                case OP_AND:
                    display_value = (display_value & operand);
                    break;
                case OP_PLUS:
                    display_value = (display_value + operand);
                    break;
                case OP_MINUS:
                    display_value = (display_value - operand);
                    break;
                case OP_TIMES:
                    display_value = (display_value * operand);
                    break;
                case OP_DIVIDE:
                    display_value = (display_value / operand);
                    break;
                }
                operation = OP_NONE;
                operand = 0;
                displaying = RESULT;
                break;
            }
        }
    }

    // cleanup and end program
    sdlx_quit(SUBSYS_VIDEO);
    printf("INFO %s: terminating\n", progname);
    return 0;
}

// -----------------  BUTTONS  ---------------------------------------

void init_button(int row, int col, int evid, char *str)
{
    button[row][col].evid = evid;
    button[row][col].str  = str; 
}


void init_buttons(void)
{
    init_button(0, 0, EVID_0+0, "0");
    init_button(0, 1, EVID_0+1, "1");
    init_button(0, 2, EVID_0+2, "2");
    init_button(0, 3, EVID_0+3, "3");
    init_button(0, 4, EVID_BACKSPACE,    "BS");

    init_button(1, 0, EVID_0+4, "4");
    init_button(1, 1, EVID_0+5, "5");
    init_button(1, 2, EVID_0+6, "6");
    init_button(1, 3, EVID_0+7, "7");
    init_button(1, 4, EVID_CHANGE_SIGN, "+/-");

    init_button(2, 0, EVID_0+8, "8");
    init_button(2, 1, EVID_0+9, "9");
    init_button(2, 2, EVID_0+0xa, "A");
    init_button(2, 3, EVID_0+0xb, "B");
    init_button(2, 4, EVID_NOT, "~");

    init_button(3, 0, EVID_0+0xc, "C");
    init_button(3, 1, EVID_0+0xd, "D");
    init_button(3, 2, EVID_0+0xe, "E");
    init_button(3, 3, EVID_0+0xf, "F");
    // avail 3, 4   use for decimal input

    init_button(4, 0, EVID_AND, "&");
    init_button(4, 1, EVID_OR, "|");
    init_button(4, 2, EVID_XOR, "^");
    init_button(4, 3, EVID_LEFT_SHIFT, "<<");
    init_button(4, 4, EVID_RIGHT_SHIFT, ">>");

    init_button(5, 0, EVID_PLUS, "+");
    init_button(5, 1, EVID_MINUS, "-");
    init_button(5, 2, EVID_TIMES, "*");
    init_button(5, 3, EVID_DIVIDE, "/");

    init_button(6, 0, EVID_CLEAR, "C");     // move first 3 to top
    init_button(6, 1, EVID_MODE,   "zzz");
    init_button(6, 2, EVID_BITS,   "zzz");
    init_button(6, 4, EVID_EQUALS, "=");
}

void draw_button(int row, int col)
{
    sdlx_loc_t *loc;
    int x, y;

    if (button[row][col].str == NULL) {
        return;
    }    

    x = BUTTONS_X_LEFT + col * BUTTONS_X_SPACING;
    y = BUTTONS_Y_TOP + row * BUTTONS_Y_SPACING;

    loc = sdlx_render_printf_xyctr(x, y, "%s", button[row][col].str);
    sdlx_register_event(loc, button[row][col].evid);
}
