/* TODO FIRST

vary serve
- position 
- direction

speeds up when motion is active 

limit paddle motion to x span


speed up ball gradually

delay before serve,  fixed 0.5 sec

*/


/* TODO

computer simulation improvement

params, such as 
- disable sound
- initial speed
- max speed
- computer skill


indicate which score is computer vs human
*/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
//#include <unistd.h>  //xxx needed?

#include <sdlx.h>
#include <utils.h>

// defines
#define DEG2RAD              (M_PI/180.0)
#define UPDATE_INTERVAL_USEC 10000  // 10 ms
#define BALL_RADIUS          50
#define PADDLE_W             300
#define PADDLE_H             50

// variables
char  *progname;
char  *data_dir;

double y_top, y_bottom;
double x, y, x_last, y_last;
double vx, vy;
double human_paddle_x, human_paddle_y;
double computer_paddle_x, computer_paddle_y;
int    human_score, computer_score;
bool   serve_needed;

// prototypes    
void computer_paddle_control(void);
void bounce_ball_off_paddle(bool is_human_paddle);
void play_tone(int freq, int duration_ms);

// -----------------  MAIN  -----------------------
    
int main(int argc, char **argv)
{
    int             rc;
    sdlx_event_t    event;
    bool            end_program = false;
    sdlx_texture_t *ball = NULL;

    // save args
    if (argc != 2) {
        printf("ERROR: data_dir arg expected\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    // init sdl video subsystem
    rc = sdlx_init(SUBSYS_VIDEO|SUBSYS_AUDIO);
    if (rc != 0) {
        printf("ERROR %s: sdlx_init failed\n", progname);
        return 1;
    }

    // initialization:
    // - create ball texture
    // - set scores to 0
    // - set the top and bottom of the display region used by the game
    // - set initial paddle locations
    // - set serve_needed flag
    ball = sdlx_create_filled_circle_texture(BALL_RADIUS, COLOR_GREEN);
    computer_score    = 0;
    human_score       = 0;
    y_top             = 0;
    y_bottom          = sdlx_win_height - 200;
    human_paddle_x    = sdlx_win_width/2;
    human_paddle_y    = y_bottom - 200;
    computer_paddle_x = sdlx_win_width/2;
    computer_paddle_y = y_top + 200;
    serve_needed      = true;

    // xxx
    x = sdlx_win_width/2;

    // init large font;
    // LARGE_FONT is defined as '10' which means 10 chars across the display
    sdlx_print_init_numchars(LARGE_FONT);

    // runtime loop
    while (!end_program) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // if serve needed then reset variables
        // xxx vary the serve
        // xxx define for serve velocity
        if (serve_needed) {
            x = computer_paddle_x;
            y = computer_paddle_y + PADDLE_H/2 + BALL_RADIUS;
            x_last = x;
            y_last = y;

            double tgtx = (random() % sdlx_win_width);
            double k = (tgtx - x) / (y_bottom - y);
            vy = 10.0 / sqrt(1 + k*k);
            vx = k * vy;
            printf("tgtx = %0.3f vx = %0.3f vy = %0.3f\n", tgtx, vx, vy);
            
            serve_needed = false;
        }

        // update ball position, using the ball x,y velocity
        x_last = x;
        y_last = y;
        x += vx;
        y += vy;

        // bounce ball off of side walls
        if (x < 0 || x > sdlx_win_width) {
            vx = -vx;
            x += vx;
        }

        // computer paddle control
        computer_paddle_control();

        // if ball has impacted human paddle then
        // bounce ball off of human paddle
        // xxx maybe use ball_right and ball_left
        double ball_bottom        = y + BALL_RADIUS;
        double ball_bottom_last   = y_last + BALL_RADIUS;
        double human_paddle_top   = human_paddle_y - (PADDLE_H/2);
        double human_paddle_left  = human_paddle_x - PADDLE_W/2;
        double human_paddle_right = human_paddle_x + PADDLE_W/2;
        if (ball_bottom >= human_paddle_top && 
            ball_bottom_last < human_paddle_top &&
            x >= human_paddle_left &&
            x <= human_paddle_right)
        {
            bounce_ball_off_paddle(true);
        }

        // if ball has impacted computer paddle then
        // bounce ball off of computer paddle
        double ball_top               = y - BALL_RADIUS;
        double ball_top_last          = y_last - BALL_RADIUS;
        double computer_paddle_bottom = computer_paddle_y + (PADDLE_H/2);
        double computer_paddle_left   = computer_paddle_x - PADDLE_W/2;
        double computer_paddle_right  = computer_paddle_x + PADDLE_W/2;
        if (ball_top <= computer_paddle_bottom && 
            ball_top_last > computer_paddle_bottom &&
            x >= computer_paddle_left &&
            x <= computer_paddle_right)
        {
            bounce_ball_off_paddle(false);
        }

        // if ball is above or below the display area then 
        //   update score
        //   play tone
        //   set serve_needed flag
        // endif
        if (y < y_top || y > y_bottom) {
            if (y < y_top) {
                human_score++; 
            } else {
                computer_score++;
            }
            play_tone(500,250);
            serve_needed = true;
        }

        // display the ball and paddles
        sdlx_render_texture(x-BALL_RADIUS, y-BALL_RADIUS, 2*BALL_RADIUS, 2*BALL_RADIUS, ball);
        sdlx_render_fill_rect(human_paddle_x-PADDLE_W/2, human_paddle_y-PADDLE_H/2, PADDLE_W, PADDLE_H, COLOR_WHITE);
        sdlx_render_fill_rect(computer_paddle_x-PADDLE_W/2, computer_paddle_y-PADDLE_H/2, PADDLE_W, PADDLE_H, COLOR_WHITE);

        // display scores
        sdlx_render_printf(0, 0, "%2d", computer_score);
        sdlx_render_printf(sdlx_win_width-2*sdlx_char_width, 0, "%d", human_score);

        // register events
        sdlx_register_event(NULL, EVID_MOTION);
        sdlx_register_control_events(NULL, NULL, "X", COLOR_WHITE, COLOR_BLACK, 0, 0, EVID_QUIT);

        // present the display
        sdlx_display_present();

        // wait for event with timeout
        // xxx motion affects interval
        sdlx_get_event(UPDATE_INTERVAL_USEC, &event);

        // process events
        switch (event.event_id) {
        case EVID_QUIT:
            end_program = true;
            break;
        case EVID_MOTION:
            human_paddle_x += event.u.motion.xrel;
            break;
        }
    }

    // cleanup and end program
    sdlx_destroy_texture(ball);
    sdlx_quit(SUBSYS_VIDEO|SUBSYS_AUDIO);
    printf("INFO %s: terminating\n", progname);
    return 0;
}

// -----------------  SUPPORT  --------------------

void computer_paddle_control(void)
{
    // xxx this may be too simple
    //computer_paddle_x = x;

    if (vy < 0) {
        //computer_paddle_x += (x - computer_paddle_x) * 0.015;  //xxx make this a computer skill from 1 to 10
        computer_paddle_x += (x - computer_paddle_x) * 0.03;
    }
}

// xxx comments
#define MAX_RAD (10.0 * DEG2RAD)
void bounce_ball_off_paddle(bool is_human_paddle)
{
    double vx1, vy1, theta;
    double paddle_x;

    if (is_human_paddle) {
        paddle_x = human_paddle_x;
        theta = (paddle_x - x) / (PADDLE_W/2) * MAX_RAD;
    } else {
        paddle_x = computer_paddle_x;
        theta = -(paddle_x - x) / (PADDLE_W/2) * MAX_RAD;
    }

    vx1 = vx*cos(theta) - vy*sin(theta);
    vy1 = vx*sin(theta) + vy*cos(theta);

    vy1 = -vy1;

    theta = -theta;
    vx = vx1*cos(theta) - vy1*sin(theta);
    vy = vx1*sin(theta) + vy1*cos(theta);

    play_tone(1000,100);
}

void play_tone(int freq, int duration_ms)
{
    static sdlx_tone_t t[2];

    t[0].freq = freq;
    t[0].intvl_ms = duration_ms;
    sdlx_audio_play_tones(t);
}

