/* TODO 
speeds up when motion is active 
limit paddle motion to x span
delay before serve,  fixed 0.5 sec
rename to paddle
indicate which score is computer vs human

params
- min and max speed
- computer skill
- auoto play
- disable sound
*/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include <sdlx.h>
#include <utils.h>

// defines
#define DEG2RAD              (M_PI/180.0)
#define RAD2DEG              (180.0/M_PI)
#define UPDATE_INTERVAL_USEC 10000  // 10 ms  xxx del one of these
#define UPDATE_INTERVAL_SEC  0.01   // 10 ms
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

double ball_speed_court_per_sec;

bool autonomous = false;

double court_pixels;

// prototypes    
void computer_paddle_control(void);
void human_paddle_control(void);
void bounce_ball_off_paddle(int which_paddle);
void play_tone(int freq, int duration_ms);

double randy(void);
double symmetric_triangular_rand(double min, double max);

#define MIN_BALL_SPEED 0.75
#define MAX_BALL_SPEED 2.75

#define HUMAN_PADDLE    0   
#define COMPUTER_PADDLE 1   
// -----------------  MAIN  -----------------------
    
int main(int argc, char **argv)
{
    int             rc;
    sdlx_event_t    event;
    bool            end_program = false;
    sdlx_texture_t *ball = NULL;

    // xxx
    srandom(time(NULL));

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

    court_pixels = (human_paddle_y - computer_paddle_y);

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
            double ball_speed_pixels_per_intvl, tgtx, k;

            // usleep(500000);  xxx delay with the ball on the computer paddle

            x = computer_paddle_x;
            y = computer_paddle_y + PADDLE_H/2 + BALL_RADIUS;
            x_last = x;
            y_last = y;

            ball_speed_court_per_sec = MIN_BALL_SPEED;
            ball_speed_pixels_per_intvl = (court_pixels * UPDATE_INTERVAL_SEC) * ball_speed_court_per_sec;

            tgtx = (random() % sdlx_win_width);
            k = (tgtx - x) / court_pixels;
            vy = ball_speed_pixels_per_intvl / sqrt(1 + k*k);
            vx = k * vy;
            
            serve_needed = false;
        }

        // update the ball velocity
        ball_speed_court_per_sec = sqrt(vx*vx + vy*vy) / (court_pixels * UPDATE_INTERVAL_SEC);
        if (ball_speed_court_per_sec < MAX_BALL_SPEED) {
            vx *= 1.0001;
            vy *= 1.0001;
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

        // xxx
        if (autonomous) {
            human_paddle_control();
        }

        // if ball has impacted human paddle then
        // bounce ball off of human paddle
        double ball_bottom        = y + BALL_RADIUS;
        double ball_bottom_last   = y_last + BALL_RADIUS;
        double human_paddle_top   = human_paddle_y - (PADDLE_H/2);
        double human_paddle_left  = human_paddle_x - PADDLE_W/2 - BALL_RADIUS;
        double human_paddle_right = human_paddle_x + PADDLE_W/2 + BALL_RADIUS;
        if (ball_bottom >= human_paddle_top && 
            ball_bottom_last < human_paddle_top &&
            x >= human_paddle_left &&
            x <= human_paddle_right)
        {
            bounce_ball_off_paddle(HUMAN_PADDLE);
        }

        // if ball has impacted computer paddle then
        // bounce ball off of computer paddle
        double ball_top               = y - BALL_RADIUS;
        double ball_top_last          = y_last - BALL_RADIUS;
        double computer_paddle_bottom = computer_paddle_y + (PADDLE_H/2);
        double computer_paddle_left   = computer_paddle_x - PADDLE_W/2 - BALL_RADIUS;
        double computer_paddle_right  = computer_paddle_x + PADDLE_W/2 + BALL_RADIUS;
        if (ball_top <= computer_paddle_bottom && 
            ball_top_last > computer_paddle_bottom &&
            x >= computer_paddle_left &&
            x <= computer_paddle_right)
        {
            bounce_ball_off_paddle(COMPUTER_PADDLE);
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

// xxx combine
void computer_paddle_control(void)
{
    double paddle_offset, k;

    if (vx < 0) {
        paddle_offset = -PADDLE_W * (fabs(vx/vy) > 0.75 ? 0.5 : 0.25);
    } else {
        paddle_offset =  PADDLE_W * (fabs(vx/vy) > 0.75 ? 0.5 : 0.25);
    }

    k = (0.10 + (ball_speed_court_per_sec - 0.75) / 10.0);
    computer_paddle_x += ((x + paddle_offset) - computer_paddle_x) * k;
}

void human_paddle_control(void)
{
    double paddle_offset, k;

    if (vx < 0) {
        paddle_offset = -PADDLE_W * (fabs(vx/vy) > 0.75 ? 0.5 : 0.25);
    } else {
        paddle_offset =  PADDLE_W * (fabs(vx/vy) > 0.75 ? 0.5 : 0.25);
    }

    k = (0.10 + (ball_speed_court_per_sec - 0.75) / 10.0);
    human_paddle_x += ((x + paddle_offset) - human_paddle_x) * k;
}

#define MAX_RAD (30.0 * DEG2RAD) //xxx name

void bounce_ball_off_paddle(int which_paddle)
{
    double vx1, vy1, theta;
    double paddle_x;

    if (which_paddle == HUMAN_PADDLE) {
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

    if (fabs(vx) > fabs(vy)) {
        double new_v = sqrt((vx*vx + vy*vy) / 2);
        vx = (vx > 0 ? new_v : -new_v);
        vy = (vy > 0 ? new_v : -new_v);
    }

    play_tone(1000,100);
}

void play_tone(int freq, int duration_ms)
{
    static sdlx_tone_t t[2];

    t[0].freq = freq;
    t[0].intvl_ms = duration_ms;
    sdlx_audio_play_tones(t);
}

// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx del xxxxxxxxxxxxxxxxxxxxxxxxxxxxx
#if 0
    static int a,b,c,d;
    for (int i = 0;i < 10000; i++) {
        //randy();
        //double x =  symmetric_triangular_rand(-PADDLE_W/2, PADDLE_W/2);
        double x =  symmetric_triangular_rand(-1000, 1000);
        if (x < -500) a++;
        else if (x < 0) b++;
        else if (x < 500) c++;
        else d++;
        //printf("%0.3f\n", x);
    }
    printf("%d %d %d %d\n", a,b,c,d);
    return 0;

double randy(void)
{
    double x = (double)random() / 0x7fffffff;
    //printf("RANDY %0.3f\n", x);
    return x;
}

double symmetric_triangular_rand(double min, double max)
{
    // Generate two uniform random numbers between 0.0 and 1.0
    double U1 = (double)rand() / (double)RAND_MAX;
    double U2 = (double)rand() / (double)RAND_MAX;
    
    // Sum them to get a triangular distribution between 0.0 and 2.0 (mode 1.0)
    double T = U1 + U2;
    
    // Scale and shift to the desired [min, max] range with mode at (min+max)/2
    return min + (max - min) * (T / 2.0);
}

double rand_uniform(double min, double max)
{
    // Generate two uniform random numbers between 0.0 and 1.0
    double T = (double)rand() / (double)RAND_MAX;
    
    // Scale and shift to the desired [min, max] range with mode at (min+max)/2
    return min + (max - min) * (T / 2.0);
}
#endif
