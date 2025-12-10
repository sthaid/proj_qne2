#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include <sdlx.h>
#include <utils.h>

// defines
#define INTERVAL 10000
#define SIZE 100

// variables
char *progname;
char *data_dir;

double x, y, vx, vy;
double x_last, y_last;

int paddle_x, paddle_y;

int computer_paddle_x, computer_paddle_y;

int computer_score, human_score;

#define PADDLE_W  300
#define PADDLE_H  50
    
void paddle(void);
void computer_paddle(void);

// -----------------  MAIN  ------------------------------------------
    
int main(int argc, char **argv)
{
    int          rc;
    sdlx_event_t event;
    bool         done = false;

    sdlx_texture_t *ball;

    // save args
    if (argc != 2) {
        printf("ERROR: data_dir arg expected\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    // init sdl video subsystem
    rc = sdlx_init(SUBSYS_VIDEO);
    if (rc != 0) {
        printf("ERROR %s: sdlx_init failed\n", progname);
        return 1;
    }

    computer_score = human_score = 0;
    ball = sdlx_create_filled_circle_texture(SIZE/2, COLOR_GREEN);
    x = sdlx_win_width/2;
    y = 0;
    vx = 0;
    vy = 10;
    paddle_x = sdlx_win_width/2;
    paddle_y = sdlx_win_height - 200;

    computer_paddle_x = sdlx_win_width/2;
    computer_paddle_y = 200;

    sdlx_print_init_numchars(LARGE_FONT);

    // runtime loop
    while (!done) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // ball
        //sdlx_render_fill_rect(x-SIZE/2, y-SIZE/2, SIZE, SIZE, COLOR_WHITE);
        sdlx_render_texture(x-SIZE/2, y-SIZE/2, SIZE, SIZE, ball);



        // bounce off of walls
        if (y > sdlx_win_height) {
            printf("comptuer scored, y=%f\n", y);
            x = sdlx_win_width/2;
            y = 0;
            vx = 0;
            vy = 10;
            x_last = x;
            y_last = y;
            computer_score++;
        } else if (y < 0) {
            printf("human scored, y=%f\n", y);
            x = sdlx_win_width/2;
            y = 0;
            vx = 0;
            vy = 10;
            x_last = x;
            y_last = y;
            human_score++;
        } else {
            if (x < 0 || x > sdlx_win_width) vx = -vx;
            //if (y < 0 || y > sdlx_win_height) vy = -vy;
            x_last = x;
            y_last = y;
            x += vx;
            y += vy;
        }


        // bounce off of paddle
        if (y_last+SIZE/2 < paddle_y && 
            y+SIZE/2 >= paddle_y && 
            x >= paddle_x-PADDLE_W/2 && 
            x <= paddle_x+PADDLE_W/2)
        {
            paddle();
        }

        computer_paddle_x = x;

        // bounce off of computer_paddle
        if (y_last-SIZE/2 > computer_paddle_y && 
            y-SIZE/2 <= computer_paddle_y && 
            x >= computer_paddle_x-PADDLE_W/2 && 
            x <= computer_paddle_x+PADDLE_W/2)
        {
            computer_paddle();
        }


        // paddle
        sdlx_render_fill_rect(paddle_x-PADDLE_W/2, paddle_y-PADDLE_H/2, PADDLE_W, PADDLE_H, COLOR_WHITE);

        // computer_paddle
        sdlx_render_fill_rect(computer_paddle_x-PADDLE_W/2, computer_paddle_y-PADDLE_H/2, PADDLE_W, PADDLE_H, COLOR_WHITE);

        sdlx_render_printf(0, 0, "%2d", computer_score);
        sdlx_render_printf(sdlx_win_width-2*sdlx_char_width, 0, "%d", human_score);

        sdlx_register_event(NULL, EVID_MOTION);

        // register control event to
        // - end program
        sdlx_register_control_events(NULL, NULL, "X", COLOR_WHITE, COLOR_BLACK, 0, 0, EVID_QUIT);



/* INPROG
move paddle
- limit motion to x span
bounce bottom of ball off paddle
*/

/*
computer paddle
score keeping
sound effects, google search for positive and negative effects
speed up ball gradually

MAYBE LATER
initial speed control

IN PROG
auto serve after 1 sec
bounce ball off paddle in varying direction
circular ball
make ball and paddle different colors
*/

        // present the display
        sdlx_display_present();

        // wait for event, timeout=INTERVAL usecs
        sdlx_get_event(INTERVAL, &event);

        // process events
        switch (event.event_id) {
        case EVID_QUIT:
            done = true;
            break;
        case EVID_MOTION:
            paddle_x += event.u.motion.xrel;
            break;
        }
    }

    // cleanup and end program
    sdlx_destroy_texture(ball);
    sdlx_quit(SUBSYS_VIDEO);
    printf("INFO %s: terminating\n", progname);
    return 0;
}

#define DEG2RAD (M_PI/180.0)
#define MAX_DEG 10.0

void paddle(void)
{
    //vy = -vy;

    double vx1, vy1;

    double theta = 10 * DEG2RAD;

    theta = (double)(paddle_x - x) / (PADDLE_W/2) * MAX_DEG;
    printf("theta %0.2f\n", theta);
    theta = theta * DEG2RAD;

    vx1 = vx*cos(theta) - vy*sin(theta);
    vy1 = vx*sin(theta) + vy*cos(theta);

    vy1 = -vy1;

    theta = -theta;

    vx = vx1*cos(theta) - vy1*sin(theta);
    vy = vx1*sin(theta) + vy1*cos(theta);
}

void computer_paddle(void)
{
    vy = -vy;
}
