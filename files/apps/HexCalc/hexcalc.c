#include <stdio.h>
#include <stdbool.h>

#include <sdlx.h>
#include <utils.h>

// defines
#define EVID_0 10
#define EVID_BACKSPACE    30
#define EVID_CHANGE_SIGN  32
#define EVID_CLEAR        33
#define EVID_AND          34
#define EVID_EQUALS       35

#define MAX_BUTTON_ROW 6
#define MAX_BUTTON_COL 5

#define BUTTONS_X_LEFT     50
#define BUTTONS_X_SPACING 200

#define BUTTONS_Y_TOP     350
#define BUTTONS_Y_SPACING 200

#define RESULT 1
#define INPUT_VALUE  2

// typedefs
typedef struct {
    int evid;
    char *str;
} button_t;

// variables
char    *progname;
char    *data_dir;
button_t button[MAX_BUTTON_ROW][MAX_BUTTON_COL];

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

        // register calculator button events
        for (int row = 0; row < MAX_BUTTON_ROW; row++) {
            for (int col = 0; col < MAX_BUTTON_COL; col++) {
                draw_button(row, col);
            }
        }
        
        sdlx_render_printf(0, 0, "%20lx", display_value);
        //sdlx_render_printf(0, ROW2Y(1), "%20ld", display_value);
        //sdlx_render_printf(0, ROW2Y(2), "%20ld", (unsigned long)display_value);



        // present the display
        sdlx_display_present();

        // wait for event, with infinite timeout
        sdlx_get_event(-1, &event);

        // process number button events
        if (event.event_id >= EVID_0 && event.event_id < EVID_0 + 16) {
            int n = event.event_id - EVID_0;
            printf("got number button %d\n", n);
            if (displaying == RESULT) {
                display_value = 0;
                displaying = INPUT_VALUE;
            }
            display_value = (display_value << 4) | n;
            continue;
        }

        // process all other events
        switch (event.event_id) {
        case EVID_BACKSPACE:
            display_value = (unsigned long)display_value >> 4;
            break;
        case EVID_CHANGE_SIGN:
            display_value = -display_value;
            break;
        case EVID_CLEAR:
            display_value = 0;
            displaying = RESULT;
            break;
        case EVID_AND:
            operand = display_value;
            operation = OP_AND;
            displaying = RESULT;
            break;
        case EVID_EQUALS:
            switch (operation) {
            case OP_AND:
                display_value = (display_value & operand);
                break;
            }
            operation = OP_NONE;
            operand = 0;
            displaying = RESULT;
            break;

        case EVID_QUIT:
            done = true;
            break;
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
    init_button(1, 0, EVID_0+4, "4");
    init_button(1, 1, EVID_0+5, "5");
    init_button(1, 2, EVID_0+6, "6");
    init_button(1, 3, EVID_0+7, "7");

    init_button(2, 0, EVID_0+8, "8");
    init_button(2, 1, EVID_0+9, "9");
    init_button(2, 2, EVID_0+0xa, "A");
    init_button(2, 3, EVID_0+0xb, "B");

    init_button(3, 0, EVID_0+0xc, "C");
    init_button(3, 1, EVID_0+0xd, "D");
    init_button(3, 2, EVID_0+0xe, "E");
    init_button(3, 3, EVID_0+0xf, "F");

    init_button(0, 4, EVID_BACKSPACE, "BS");
    init_button(1, 4, EVID_CHANGE_SIGN, "+/-");

    init_button(4, 0, EVID_AND, "&");

    init_button(5, 0, EVID_CLEAR, "C");
    init_button(5, 4, EVID_EQUALS, "=");
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
