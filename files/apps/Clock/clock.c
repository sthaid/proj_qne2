#include <stdio.h>  // xxx
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <sdlx.h>
#include <utils.h>

#include "apps/Clock/common.h"

// xxx 
// - maybe pixels shoudl be unsigned int
// - options to:
//   - 24 hr time

// defines
#define XCTR_CLOCK 500
#define YCTR_CLOCK 600
#define W_CLOCK    1000
#define H_CLOCK    1000

// variables
//static char *progname;
//static char *data_dir;

static char *day[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static char *month[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

// prototypes
static void draw_analog_clock_face(void);
static void draw_analog_clock_hands(struct tm *tm);
int sunrise_sunset(int year, int month, int day, time_t *trise, time_t *tset); //xxx static

// xxx
//void test(void);
//void test2(void);

// -----------------  MAIN  ------------------------------------------

int main(int argc, char **argv)
{
    sdlx_event_t    event;
    int             rc, y;
    bool            quit = false;
    time_t          t;
    struct tm       tm;
    char            sunrise[50], sunset[50], midday[50];

    // save arg values
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s, wXh=%d %d\n", 
           progname, data_dir, sdlx_win_width, sdlx_win_height);

    // xxx
    sunrise_sunset_calc(sunrise, sunset, midday);

    // init sdl video subsystem
    rc = sdlx_init(SUBSYS_VIDEO);
    if (rc != 0) {
        printf("ERROR %s: sdlx_init failed\n", progname);
        return 1;
    }

    // runtime loop
    while (!quit) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // register control event to end program
        sdlx_register_control_events(NULL, NULL, "X", COLOR_BLACK, 0, 0, EVID_QUIT);

        // xxx get the time
        t = time(NULL);
        localtime_r(&t, &tm);

        // display the analog clock  xxx just one call
        draw_analog_clock_face();
        draw_analog_clock_hands(&tm);

        // display the date and time below the analog clock, example:
        // 01:30:00 PM EDT  or  13:30:00 EDT
        // Wed Oct 21 2025
        y = YCTR_CLOCK + H_CLOCK / 2 + 2 * sdlx_char_height;
        sdlx_render_printf_xyctr(
                sdlx_win_width/2, y, 
                "%02d:%02d:%02d %s",
                tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_zone);
        y += 1.5 * sdlx_char_height;
        sdlx_render_printf_xyctr(
                sdlx_win_width/2, y, 
                "%s %s %d %d",
                day[tm.tm_wday], month[tm.tm_mon], tm.tm_mday, tm.tm_year+1900);

        // display sunrise, midday, sunset times, example:
        // RISE     MID      SET
        // 07:00   12:00   17:00
        // xxx
    
        // present the display
        sdlx_display_present();

        // wait for an event with 1 s timeout;
        // if no event, then redraw display
        sdlx_get_event(1000000, &event);
        if (event.event_id == -1) {
            continue;
        }

        // process events
        switch (event.event_id) {
        case EVID_QUIT:
            quit = true;
            break;
        }
    }

    // cleanup and end program
    //xxx cleanup_analog_clock();
    sdlx_quit(SUBSYS_VIDEO);
    printf("INFO %s: terminating\n", progname);
    return 0;
}

// -----------------  ANALOG CLOCK FACE  -----------------------------

// xxx save the face in a texture

static void draw_analog_clock_face(void)
{
    int hour, x, y;

    sdlx_render_fill_rect(0, 100, 1000, 1000, COLOR_WHITE);

    sdlx_print_init(DEFAULT_FONT, COLOR_BLACK, COLOR_WHITE);

    for (hour = 1; hour <= 12; hour++) {
        x = 500 + 400 * sin(hour * 30 * (M_PI / 180));  //xxx use deines
        y = 600 - 400 * cos(hour * 30 * (M_PI / 180));
        sdlx_render_printf_xyctr(x, y, "%d", hour);
    }
}

// -----------------  ANALOG CLOCK HANDS -----------------------------

#define W_HH  34
#define H_HH  280
#define O_HH  40

#define W_MH  17
#define H_MH  375
#define O_MH  60

#define W_SH  4
#define H_SH  425
#define O_SH  100

sdlx_texture_t *hour_hand;
sdlx_texture_t *minute_hand;
sdlx_texture_t *second_hand;

static sdlx_texture_t * create_rectangle_texture(int w, int h, int color);

static void draw_analog_clock_hands(struct tm *tm)
{
    static bool first_call = true;

    double hour_hand_angle, minute_hand_angle, second_hand_angle;
    long   secs;

    if (first_call) {
        hour_hand = create_rectangle_texture(W_HH, H_HH, COLOR_BLACK);
        minute_hand = create_rectangle_texture(W_MH, H_MH, COLOR_BLACK);  // xxx free these
        second_hand = create_rectangle_texture(W_SH, H_SH, COLOR_RED);  // xxx free these
        first_call = false;
    }

    secs = 3600 * tm->tm_hour + 60 * tm->tm_min + tm->tm_sec;

    hour_hand_angle   = secs * (360. / (12 * 3600));
    minute_hand_angle = secs * (360. / 3600);
    second_hand_angle = secs * (360. / 60);

    sdlx_render_texture_ex2(XCTR_CLOCK-(W_HH/2), YCTR_CLOCK-H_HH+O_HH, 
                            W_HH, H_HH, 
                            hour_hand_angle, 
                            W_HH/2, H_HH-O_HH,
                            hour_hand);

    sdlx_render_texture_ex2(XCTR_CLOCK-(W_MH/2), YCTR_CLOCK-H_MH+O_MH, 
                            W_MH, H_MH, 
                            minute_hand_angle, 
                            W_MH/2, H_MH-O_MH,
                            minute_hand);

    sdlx_render_texture_ex2(XCTR_CLOCK-(W_SH/2), YCTR_CLOCK-H_SH+O_SH, 
                            W_SH, H_SH, 
                            second_hand_angle,
                            W_SH/2, H_SH-O_SH,
                            second_hand);

    sdlx_render_point(XCTR_CLOCK, YCTR_CLOCK, COLOR_RED, 9);
}
    
sdlx_texture_t * create_rectangle_texture(int w, int h, int color)
{
    unsigned int *pixels = malloc(w * h * BYTES_PER_PIXEL);
    sdlx_texture_t *t;

    pixels = malloc(w * h * BYTES_PER_PIXEL);
    for (int i = 0; i < w * h; i++) {
        pixels[i] = color;
    }
    t = sdlx_create_texture_from_pixels((unsigned char*)pixels, w, h);
    free(pixels);

    return t;
}

#if 0
// -----------------  SUNRISE SUNSET METHOD 2  -----------------------

#define JD2000 2451545.0

#define SIND(x)   (sin((x)*DEG2RAD))
#define COSD(x)   (cos((x)*DEG2RAD))
#define TAND(x)   (tan((x)*DEG2RAD))
#define ACOSD(x)  (acos(x)*RAD2DEG)
#define ASIND(x)  (asin(x)*RAD2DEG)

#define RAD2DEG (180. / M_PI)
#define DEG2RAD (M_PI / 180.)

static void jd2ymdh(double jd, int *year, int *month, int *day, double *hour);
static double ymdh2jd(int yr, int mn, int day, double hour); //xxx name
static void hr2hms(double hr, int * hour, int * minute, int * seconds);

static int run_curl(char *url, char *filename)
{
    char cmd[500];
    int  ret;

    //printf("INFO %s: url = %s\n", progname, url);

    util_delete_file(data_dir, filename);

    sprintf(cmd, "curl -s %s > %s/%s", url, data_dir, "curl.out");

    ret = system(cmd);
    if (ret != 0) {
        printf("ERROR %s: curl failed for url '%s'\n", progname, url);
        return -1;
    }

    return 0;
}

static void *get_json_root(char *filename)
{
    char *str;
    void *root;
    int   len;

    str = util_read_file(data_dir, filename, &len);
    if (str == NULL) {
        printf("ERROR %s: failed to read file %s/%s\n",
               progname, data_dir, filename);
        return NULL;
    }

    root = util_json_parse(str);
    if (root == NULL) {
        printf("ERROR %s: failed to parse json\n", progname);
        free(str);
        return NULL;
    }

    free(str);

    return root;
}


void test2(void)
{
    char         curl_url[200];
    json_value_t rise, set;
    int          ret;
    double       latitude, longitude;
    void        *root;

    // get location
    util_get_location(&latitude, &longitude, NULL);
    if (latitude == INVALID_NUMBER || longitude == INVALID_NUMBER) {
        printf("ERROR %s: failed to get gps location\n", progname);
        goto done;
    }

    // xxx
    sprintf(curl_url, "\"https://api.sunrise-sunset.org/json?lat=%0.4f&lng=%0.4f\"", latitude, longitude);
    ret = run_curl(curl_url, "curl.out");
    if (ret != 0) {
        printf("ERROR %s: run_curl failed\n", progname);
        goto done;
    }

    // xxx
    root = get_json_root("curl.out");
    if (root == NULL) {
        printf("ERROR %s: json parse failed\n", progname);
        goto done;
    }

    rise = *util_json_get_value(root, "results", "sunrise", NULL);
    set = *util_json_get_value(root, "results", "sunset", NULL);
    printf("RISE %s\n", rise.u.string);
    printf("SET  %s\n", set.u.string);

done:
    util_json_free(root);
}

void test(void)
{
    int year, month, day;
    double hr;
    double jd;
    time_t trise, tset;
    struct tm tmrise, tmset;
    int rc;

    jd2ymdh(JD2000, &year, &month, &day, &hr);
    printf("%d %d %d %f\n", year, month, day, hr);

    jd = ymdh2jd(year, month, day, hr);
    printf("%f %f %f\n", JD2000, jd, jd - JD2000);

    time_t t;
    struct tm tm;
    t = time(NULL);
    gmtime_r(&t, &tm);
    year = tm.tm_year + 1900;
    month = tm.tm_mon + 1;
    day = tm.tm_mday;

    rc = sunrise_sunset(year, month, day, &trise, &tset);
    if (rc == 0) {
        localtime_r(&trise, &tmrise);
        localtime_r(&tset, &tmset);
        printf("rise: %02d/%02d/%d %02d:%02d:%02d   set: %02d/%02d/%d %02d:%02d:%02d\n",
           tmrise.tm_mon+1, tmrise.tm_mday, tmrise.tm_year+1900,
           tmrise.tm_hour, tmrise.tm_min, tmrise.tm_sec,
           tmset.tm_mon+1, tmset.tm_mday, tmset.tm_year+1900,
           tmset.tm_hour, tmset.tm_min, tmset.tm_sec);
    } else {
        printf("ERROR \n");
    }
}

// https://en.wikipedia.org/wiki/Sunrise_equation#Hour_angle
int sunrise_sunset(int year, int month, int day, time_t *trise, time_t *tset) //xxx static
{
    int              n;
    double           jd, jstar, M, C, lambda, jtransit, declination, hour_angle, jset, jrise, hr;
    struct tm        tm;
    double latitude, longitude;

    // get location
    util_get_location(&latitude, &longitude, NULL);
    if (latitude == INVALID_NUMBER || longitude == INVALID_NUMBER) {
        printf("ERROR %s: failed to get gps location\n", progname);
        return -1;
    }

    // get julian date
    jd = ymdh2jd(year, month, day, 0);

    // calculate number of julian days since JD2000 epoch
    n = ceil(jd - JD2000 + 0.0008);

    // mean solar noon
    jstar = n - longitude / 360;

    // solar mean anomaly
    M = (357.5291 + 0.98560028 * jstar);
    if (M < 0) {
        printf("ERROR %s: BUG M < 0\n", progname);  // xxx ret error?
        return -1;
    }
    while (M >= 360) M -= 360; 

    // equation of the center
    C = 1.9148 * SIND(M) + 0.0200 * SIND(2*M) + 0.0003 * SIND(3*M);

    // ecliptic longitude
    lambda = (M +  C + 180 + 102.9372);
    if (lambda < 0) {
        printf("ERROR %s: BUG lambda < 0\n", progname);  // xxx ret error?
        return -1;
    }
    while (lambda >= 360) lambda -= 360;

    // solar transit
    jtransit = JD2000 + jstar + 0.0053 * SIND(M) - 0.0069 * SIND(2*lambda);

    // declination of the sun
    declination = ASIND(SIND(lambda) * SIND(23.44));

#if 0
    // hour angle for sun center and no refraction correction
    hour_angle = ACOSD(-TAND(latitude) * TAND(declination));
#else
    // hour angle with correction for refraction and disc diameter
    hour_angle = ACOSD( (SIND(-0.83) - SIND(latitude) * SIND(declination)) /
                       (COSD(latitude) * COSD(declination)));
#endif

    // calculate sunrise and sunset
    jset = jtransit + hour_angle / 360;
    jrise = jtransit - hour_angle / 360;

    // convert jrise to linux time
    if (trise) {
        int hour, minute, seconds;

        jd2ymdh(jrise, &year, &month, &day, &hr);
        hr2hms(hr, &hour, &minute, &seconds);
        memset(&tm,0,sizeof(tm));
        tm.tm_sec   = seconds;
        tm.tm_min   = minute;
        tm.tm_hour  = hour;
        tm.tm_mday  = day;
        tm.tm_mon   = month - 1;     // 0 to 11
        tm.tm_year  = year - 1900;   // based 1900
        *trise = timegm(&tm);
    }

    // convert jset to linux time
    if (tset) {
        int hour, minute, seconds;

        jd2ymdh(jset, &year, &month, &day, &hr);
        hr2hms(hr, &hour, &minute, &seconds);
        memset(&tm,0,sizeof(tm));
        tm.tm_sec   = seconds;
        tm.tm_min   = minute;
        tm.tm_hour  = hour;
        tm.tm_mday  = day;
        tm.tm_mon   = month - 1;     // 0 to 11
        tm.tm_year  = year - 1900;   // based 1900
        *tset = timegm(&tm);
    }

    // success
    return 0;
}

// based on https://aa.usno.navy.mil/faq/docs/JD_Formula.php
static void jd2ymdh(double jd, int *year, int *month, int *day, double *hour)
{
    // 
    // COMPUTES THE GREGORIAN CALENDAR DATE (YEAR,MONTH,DAY)
    // GIVEN THE JULIAN DATE (JD).
    // 

    int JD = jd + 0.5;
    int I,J,K,L,N;

    L= JD+68569;
    N= 4*L/146097;
    L= L-(146097*N+3)/4;
    I= 4000*(L+1)/1461001;
    L= L-1461*I/4+31;
    J= 80*L/2447;
    K= L-2447*J/80;
    L= J/11;
    J= J+2-12*L;
    I= 100*(N-49)+I+L;

    *year= I;
    *month= J;
    *day = K;

    double jd0 = ymdh2jd(*year, *month, *day, 0);
    *hour = (jd - jd0) * 24;
}

// https://idlastro.gsfc.nasa.gov/ftp/pro/astro/jdcnv.pro
// Converts Gregorian dates to Julian days
static double ymdh2jd(int yr, int mn, int day, double hour)
{
    int L, julian;

    L = (mn-14)/12;    // In leap years, -1 for Jan, Feb, else 0
    julian = day - 32075 + 1461*(yr+4800+L)/4 +
             367*(mn - 2-L*12)/12 - 3*((yr+4900+L)/100)/4;

    return (double)julian + (hour/24.) - 0.5;
}

static void hr2hms(double hr, int * hour, int * minute, int * seconds)
{       
    double secs = hr * 3600;
        
    *hour = secs / 3600;
    secs -= 3600 * *hour;
    *minute = secs / 60;
    secs -= *minute * 60;
    *seconds = nearbyint(secs);
}  
#endif
