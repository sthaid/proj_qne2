#ifndef __SDL_H__
#define __SDL_H__

// --------------------
// rendering
// --------------------

// https://www.w3schools.com/colors/colors_converter.asp
#define BYTES_PER_PIXEL  4
#define COLOR_BLACK      (   0  |    0<<8 |    0<<16 |  255<<24 )
#define COLOR_WHITE      ( 255  |  255<<8 |  255<<16 |  255<<24 )
#define COLOR_RED        ( 255  |    0<<8 |    0<<16 |  255<<24 )
#define COLOR_ORANGE     ( 255  |  128<<8 |    0<<16 |  255<<24 )
#define COLOR_YELLOW     ( 255  |  255<<8 |    0<<16 |  255<<24 )
#define COLOR_GREEN      (   0  |  255<<8 |    0<<16 |  255<<24 )
#define COLOR_BLUE       (   0  |    0<<8 |  255<<16 |  255<<24 )
#define COLOR_INDIGO     (  75  |    0<<8 |  130<<16 |  255<<24 )
#define COLOR_VIOLET     ( 238  |  130<<8 |  238<<16 |  255<<24 )
#define COLOR_PURPLE     ( 127  |    0<<8 |  255<<16 |  255<<24 )
#define COLOR_LIGHT_BLUE (   0  |  255<<8 |  255<<16 |  255<<24 )
#define COLOR_PINK       ( 255  |  105<<8 |  180<<16 |  255<<24 )
#define COLOR_TEAL       (   0  |  128<<8 |  128<<16 |  255<<24 )
#define COLOR_LIGHT_GRAY ( 192  |  192<<8 |  192<<16 |  255<<24 )
#define COLOR_GRAY       ( 128  |  128<<8 |  128<<16 |  255<<24 )
#define COLOR_DARK_GRAY  (  64  |   64<<8 |   64<<16 |  255<<24 )

#define EVID_SWIPE_RIGHT       9990
#define EVID_SWIPE_LEFT        9991
#define EVID_MOTION            9992
#define EVID_KEYBD             9993
#define EVID_QUIT              9999  // xxx review where this is used

#define ROW2Y(r) ((r) * sdl_char_height)
#define COL2X(c) ((c) * sdl_char_width)

//
// typedefs
//

typedef struct {
    int x, y, w, h;
} sdl_loc_t;

typedef struct {
    int x, y;
} sdl_point_t;

typedef struct sdl_texture sdl_texture_t;

typedef struct {
    int event_id;
    union {
        struct {
            double x, y, xrel, yrel;
        } motion;
        struct {
            int ch;
        } keybd;
    } u;
} sdl_event_t;

#define PIXELS_MAGIC 0x11223344
typedef struct {
    int magic;
    int struct_len;
    int w;
    int h;
    int pixels[0];
} sdl_pixels_t;

typedef struct {
    int ptsize;
    int char_width;
    int char_height;
    int bg_color;
    int fg_color;
} sdl_print_state_t;

//
// global variables
//

extern int sdl_win_width;
extern int sdl_win_height;
extern int sdl_char_width;
extern int sdl_char_height;

//
// prototypes
//

// sdl initialization and termination, must be done once
int sdl_init(void);
void sdl_exit(void);

// display init and present, must be done for every display update
void sdl_display_init(int color);
void sdl_display_present(void);

// event registration and query
void sdl_register_event(sdl_loc_t *loc, int event_id);
void sdl_register_control_events(char *evstr1, char *evstr2, char *evstr3, int bg_color,
                                 int evid1, int evid2, int evid3);
void sdl_get_event(long timeout_us, sdl_event_t *event);

// create colors
int sdl_create_color(int r, int g, int b, int a);
int sdl_scale_color(int color, double inten);
int sdl_wavelength_to_color(int wavelength);

// render text
void sdl_print_init(double numchars, int fg_color, int bg_color);
void sdl_print_init_color(int fg_color, int bg_color);
void sdl_print_save(sdl_print_state_t *save);
void sdl_print_restore(sdl_print_state_t *restore);
sdl_loc_t *sdl_render_text(int x, int y, char *str);
sdl_loc_t *sdl_render_printf(int x, int y, char *fmt, ...) __attribute__ ((format (printf, 3, 4)));
sdl_loc_t *sdl_render_text_xyctr(int x, int y, char *str);
sdl_loc_t *sdl_render_printf_xyctr(int x, int y, char *fmt, ...) __attribute__ ((format (printf, 3, 4)));
void sdl_render_multiline_text(int y_top, int y_display_begin, int y_display_end, char * str);
void sdl_render_multiline_text_2(int y_top, int y_display_begin, int y_display_end, char **lines, int n);

// render rectangle, lines, circles, points
void sdl_render_rect(int x, int y, int w, int h, int line_width, int color);
void sdl_render_fill_rect(int x, int y, int w, int h, int color);
void sdl_render_line(int x1, int y1, int x2, int y2, int color);
void sdl_render_lines(sdl_point_t *points, int count, int color);
void sdl_render_circle(int x_ctr, int y_ctr, int radius, int line_width, int color);
void sdl_render_point(int x, int y, int color, int point_size);
void sdl_render_points(sdl_point_t *points, int count, int color, int point_size);

// render using textures
sdl_texture_t *sdl_create_texture_from_pixels(sdl_pixels_t *pixels);
sdl_texture_t *sdl_create_filled_circle_texture(int radius, int color);
sdl_texture_t *sdl_create_text_texture(char *str);
void sdl_render_texture(int x, int y, int w, int h, double angle, sdl_texture_t *texture);
void sdl_destroy_texture(sdl_texture_t *texture);
void sdl_query_texture(sdl_texture_t *texture, int *w, int *h);
sdl_pixels_t *sdl_read_display_pixels(int x, int y, int w, int h);

// get string, from virtual keyboard when on Android
char *sdl_get_input_str(char *prompt, bool numeric_keybd, int bg_color);

// --------------------
// audio
// --------------------

#define AUDIO_REQ_STOP     1
#define AUDIO_REQ_PAUSE    2
#define AUDIO_REQ_UNPAUSE  3

#define AUDIO_STATE_IDLE           0
#define AUDIO_STATE_PLAY_FILE      1
#define AUDIO_STATE_PLAY_TONES     2
#define AUDIO_STATE_RECORD         3
#define AUDIO_STATE_RECORD_APPEND  4

typedef struct {
    short freq;
    short intvl;
} sdl_tone_t;

typedef struct {
    int  state;
    bool paused;
    int  processed_secs;
    int  total_secs;
    int  volume;
    char filename[100];
} sdl_audio_state_t;

int sdl_audio_play(char *filename);
int sdl_audio_record(char *filename, int max_duration_secs, int auto_stop_secs, bool append);
int sdl_audio_play_tones(int time_units_ms, sdl_tone_t *tones);

void sdl_audio_ctl(int req);
void sdl_audio_state(sdl_audio_state_t * state);

void sdl_audio_print_devices_info(void);
void sdl_audio_create_test_file(char *filename, int duration_secs, int freq);

// --------------------
// sensors
// --------------------

#define ASENSOR_TYPE_ACCELEROMETER       1
#define ASENSOR_TYPE_MAGNETIC_FIELD      2
#define ASENSOR_TYPE_GYROSCOPE           4
#define ASENSOR_TYPE_LIGHT               5
#define ASENSOR_TYPE_PRESSURE            6
#define ASENSOR_TYPE_PROXIMITY           8
#define ASENSOR_TYPE_GRAVITY             9
#define ASENSOR_TYPE_LINEAR_ACCELERATION 10
#define ASENSOR_TYPE_ROTATION_VECTOR     11
#define ASENSOR_TYPE_RELATIVE_HUMIDITY   12
#define ASENSOR_TYPE_AMBIENT_TEMPERATURE 13
#define ASENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED 14
#define ASENSOR_TYPE_GAME_ROTATION_VECTOR 15
#define ASENSOR_TYPE_GYROSCOPE_UNCALIBRATED 16
#define ASENSOR_TYPE_SIGNIFICANT_MOTION 17
#define ASENSOR_TYPE_STEP_DETECTOR 18
#define ASENSOR_TYPE_STEP_COUNTER 19
#define ASENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR 20
#define ASENSOR_TYPE_HEART_RATE 21
#define ASENSOR_TYPE_POSE_6DOF 28
#define ASENSOR_TYPE_STATIONARY_DETECT 29
#define ASENSOR_TYPE_MOTION_DETECT 30
#define ASENSOR_TYPE_HEART_BEAT 31
#define ASENSOR_TYPE_DYNAMIC_SENSOR_META 32
#define ASENSOR_TYPE_ADDITIONAL_INFO 33
#define ASENSOR_TYPE_LOW_LATENCY_OFFBODY_DETECT 34
#define ASENSOR_TYPE_ACCELEROMETER_UNCALIBRATED 35
#define ASENSOR_TYPE_HINGE_ANGLE 36
#define ASENSOR_TYPE_HEAD_TRACKER 37
#define ASENSOR_TYPE_ACCELEROMETER_LIMITED_AXES 38
#define ASENSOR_TYPE_GYROSCOPE_LIMITED_AXES 39
#define ASENSOR_TYPE_ACCELEROMETER_LIMITED_AXES_UNCALIBRATED 40
#define ASENSOR_TYPE_GYROSCOPE_LIMITED_AXES_UNCALIBRATED 41
#define ASENSOR_TYPE_HEADING 42

typedef struct {
    int   id;
    int   sdltype;
    int   nptype;
    char *name;
} sdl_sensor_info_t;

int sdl_sensor_init_private(void);

sdl_sensor_info_t *sdl_sensor_get_info_tbl(int *num_sensors);
void *sdl_sensor_open_by_nptype(int nptype);
void *sdl_sensor_open_by_id(int id);
void sdl_sensor_close(void *sensor);
int sdl_sensor_read(void *sensor, double *values, int num_values);

#endif
