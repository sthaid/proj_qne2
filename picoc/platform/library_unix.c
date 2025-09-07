#include "../interpreter.h"
#include <sdl.h>
#include <utils.h>

struct StdVararg
{
    struct Value **Param;
    int NumArgs;
};

int StdioBasePrintf(struct ParseState *Parser, FILE *Stream, char *StrOut,
    int StrOutLen, char *Format, struct StdVararg *Args);

// -----------------  SDL PLATFORM ROUTINES  ----------------------------

//
// sdl initialization and termination, must be done once
//

void Sdl_init (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int ret;

    ret = sdl_init();

    ReturnValue->Val->Integer = ret;
}

void Sdl_exit (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_exit();
}

//
// display init and present, must be done for every display update
//

void Sdl_display_init (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int color = Param[0]->Val->Integer;

    sdl_display_init(color);
}

void Sdl_display_present (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_display_present();
}

//
// event registration and query
//

void Sdl_register_event (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_loc_t *loc      = (sdl_loc_t*)Param[0]->Val->Pointer;
    int        event_id = Param[1]->Val->Integer;

    sdl_register_event(loc, event_id);
}

void Sdl_register_control_events (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *evstr1   = (char*)Param[0]->Val->Pointer;
    char *evstr2   = (char*)Param[1]->Val->Pointer;
    char *evstr3   = (char*)Param[2]->Val->Pointer;
    int   bg_color = Param[3]->Val->Integer;
    int   evid1    = Param[4]->Val->Integer;
    int   evid2    = Param[5]->Val->Integer;
    int   evid3    = Param[6]->Val->Integer;

    sdl_register_control_events(evstr1, evstr2, evstr3, bg_color, evid1, evid2, evid3);
}

void Sdl_get_event (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    long         timeout_us = Param[0]->Val->LongInteger;
    sdl_event_t *event      = Param[1]->Val->Pointer;

    sdl_get_event(timeout_us, event);
}

//
// create colors
//

void Sdl_create_color (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int r = Param[0]->Val->Integer;
    int g = Param[1]->Val->Integer;
    int b = Param[2]->Val->Integer;
    int a = Param[3]->Val->Integer;
    int color;

    color = sdl_create_color(r, g, b, a);
    
    ReturnValue->Val->Integer = color;
}

void Sdl_scale_color (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int color = Param[0]->Val->Integer;
    double inten = Param[1]->Val->FP;
    int scaled_color;

    scaled_color = sdl_scale_color(color, inten);

    ReturnValue->Val->Integer = scaled_color;
}

void Sdl_wavelength_to_color (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int wavelength = Param[0]->Val->Integer;
    int color;

    color = sdl_wavelength_to_color(wavelength);

    ReturnValue->Val->Integer = color;
}

//
// render text
//

void Sdl_print_init (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    double numchars = Param[0]->Val->FP;
    int    fg_color = Param[1]->Val->Integer;
    int    bg_color = Param[2]->Val->Integer;

    sdl_print_init(numchars, fg_color, bg_color);
}

void Sdl_print_init_color (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int fg_color = Param[0]->Val->Integer;
    int bg_color = Param[1]->Val->Integer;

    sdl_print_init_color(fg_color, bg_color);
}

void Sdl_print_save (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_print_state_t *print_state = (sdl_print_state_t*)Param[0]->Val->Pointer;

    sdl_print_save(print_state);
}

void Sdl_print_restore (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_print_state_t *print_state = (sdl_print_state_t*)Param[0]->Val->Pointer;

    sdl_print_restore(print_state);
}

void Sdl_render_text (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int         x   = Param[0]->Val->Integer;
    int         y   = Param[1]->Val->Integer;
    char       *str = Param[2]->Val->Pointer;
    sdl_loc_t  *loc;

    loc = sdl_render_text(x, y, str);

    ReturnValue->Val->Pointer = loc;
}

void Sdl_render_printf (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int   x   = Param[0]->Val->Integer;
    int   y   = Param[1]->Val->Integer;
    char *fmt = Param[2]->Val->Pointer;

    struct StdVararg PrintfArgs;
    char             str[200] = "";
    sdl_loc_t       *loc;

    PrintfArgs.Param = Param + 2;
    PrintfArgs.NumArgs = NumArgs - 3;
    StdioBasePrintf(Parser, NULL, str, sizeof(str), fmt, &PrintfArgs);

    loc = sdl_render_text(x, y, str);

    ReturnValue->Val->Pointer = loc;
}

void Sdl_render_text_xyctr (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int         x   = Param[0]->Val->Integer;
    int         y   = Param[1]->Val->Integer;
    char       *str = Param[2]->Val->Pointer;
    sdl_loc_t  *loc;

    loc = sdl_render_text_xyctr(x, y, str);

    ReturnValue->Val->Pointer = loc;
}

void Sdl_render_printf_xyctr (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int   x   = Param[0]->Val->Integer;
    int   y   = Param[1]->Val->Integer;
    char *fmt = Param[2]->Val->Pointer;

    struct StdVararg PrintfArgs;
    char             str[200] = "";
    sdl_loc_t       *loc;

    PrintfArgs.Param = Param + 2;
    PrintfArgs.NumArgs = NumArgs - 3;
    StdioBasePrintf(Parser, NULL, str, sizeof(str), fmt, &PrintfArgs);

    loc = sdl_render_text_xyctr(x, y, str);

    ReturnValue->Val->Pointer = loc;
}

void Sdl_render_multiline_text (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int   y_top           = Param[0]->Val->Integer;
    int   y_display_begin = Param[1]->Val->Integer;
    int   y_display_end   = Param[2]->Val->Integer;
    char *str             = Param[3]->Val->Pointer;

    sdl_render_multiline_text(y_top, y_display_begin, y_display_end, str);
}

void Sdl_render_multiline_text_2 (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int    y_top           = Param[0]->Val->Integer;
    int    y_display_begin = Param[1]->Val->Integer;
    int    y_display_end   = Param[2]->Val->Integer;
    char **lines           = Param[3]->Val->Pointer;
    int    n               = Param[4]->Val->Integer;

    sdl_render_multiline_text_2(y_top, y_display_begin, y_display_end, lines, n);
}

//
// render rectangle, lines, circles, points
//

void Sdl_render_rect (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int   x          = Param[0]->Val->Integer;
    int   y          = Param[1]->Val->Integer;
    int   w          = Param[2]->Val->Integer;
    int   h          = Param[3]->Val->Integer;
    int   line_width = Param[4]->Val->Integer;
    int   color      = Param[5]->Val->Integer;

    sdl_render_rect(x, y, w, h, line_width, color);
}

void Sdl_render_fill_rect (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int   x          = Param[0]->Val->Integer;
    int   y          = Param[1]->Val->Integer;
    int   w          = Param[2]->Val->Integer;
    int   h          = Param[3]->Val->Integer;
    int   color      = Param[4]->Val->Integer;

    sdl_render_fill_rect(x, y, w, h, color);
}

void Sdl_render_line (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int x1    = Param[0]->Val->Integer;
    int y1    = Param[1]->Val->Integer;
    int x2    = Param[2]->Val->Integer;
    int y2    = Param[3]->Val->Integer;
    int color = Param[4]->Val->Integer;

    sdl_render_line(x1, y1, x2, y2, color);
}

void Sdl_render_lines (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_point_t *points = (sdl_point_t*)Param[0]->Val->Pointer;
    int count           = Param[1]->Val->Integer;
    int color           = Param[2]->Val->Integer;

    sdl_render_lines(points, count, color);
}

void Sdl_render_circle (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int  x_ctr      = Param[0]->Val->Integer;
    int  y_ctr      = Param[1]->Val->Integer;
    int  radius     = Param[2]->Val->Integer;
    int  line_width = Param[3]->Val->Integer;
    int  color      = Param[4]->Val->Integer;

    sdl_render_circle(x_ctr, y_ctr, radius, line_width, color);
}

void Sdl_render_point (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int x          = Param[0]->Val->Integer;
    int y          = Param[1]->Val->Integer;
    int color      = Param[2]->Val->Integer;
    int point_size = Param[3]->Val->Integer;

    sdl_render_point(x, y, color, point_size);
}

void Sdl_render_points (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_point_t *points = (sdl_point_t*)Param[0]->Val->Pointer;
    int count           = Param[1]->Val->Integer;
    int color           = Param[2]->Val->Integer;
    int point_size      = Param[3]->Val->Integer;

    sdl_render_points(points, count, color, point_size);
}

//
// render using textures
//

void Sdl_create_texture_from_pixels (struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    sdl_pixels_t *pixels = Param[0]->Val->Pointer;
    sdl_texture_t *texture;

    texture = sdl_create_texture_from_pixels(pixels);
    ReturnValue->Val->Pointer = (char*)texture; 
}

void Sdl_create_filled_circle_texture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int radius = Param[0]->Val->Integer;
    int color  = Param[1]->Val->Integer;
    sdl_texture_t *texture;

    texture = sdl_create_filled_circle_texture(radius, color);
    ReturnValue->Val->Pointer = (char*)texture; 
}

void Sdl_create_text_texture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *str = (char*)Param[0]->Val->Pointer;
    sdl_texture_t *texture;

    texture = sdl_create_text_texture(str);
    ReturnValue->Val->Pointer = (char*)texture; 
}

void Sdl_render_texture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int            x       = Param[0]->Val->Integer;
    int            y       = Param[1]->Val->Integer;
    int            w       = Param[2]->Val->Integer;
    int            h       = Param[3]->Val->Integer;
    double         angle   = Param[4]->Val->FP;
    sdl_texture_t *texture = (sdl_texture_t*)Param[5]->Val->Pointer;

    sdl_render_texture(x, y, w, h, angle, texture);
}

void Sdl_destroy_texture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_texture_t *texture = (sdl_texture_t*)Param[0]->Val->Pointer;

    sdl_destroy_texture(texture);
}

void Sdl_query_texture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_texture_t *texture = (sdl_texture_t*)Param[0]->Val->Pointer;
    int           *width   = (int*)Param[1]->Val->Pointer;
    int           *height  = (int*)Param[2]->Val->Pointer;

    sdl_query_texture(texture, width, height);
}

void Sdl_read_display_pixels (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int x = Param[0]->Val->Integer;
    int y = Param[1]->Val->Integer;
    int w = Param[2]->Val->Integer;
    int h = Param[3]->Val->Integer;
    sdl_pixels_t *pixels;

    pixels = sdl_read_display_pixels(x, y, w, h);
    ReturnValue->Val->Pointer = (char*)pixels; 
}

//
// audio
//

void Sdl_audio_play (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *filename = Param[0]->Val->Pointer;
    int rc;

    rc = sdl_audio_play(filename);
    ReturnValue->Val->Integer = rc; 
}

void Sdl_audio_record (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *filename           = Param[0]->Val->Pointer;
    int   max_duration_secs  = Param[1]->Val->Integer;
    int   auto_stop_secs     = Param[2]->Val->Integer;
    bool  append             = Param[3]->Val->Integer;
    int   rc;

    rc = sdl_audio_record(filename, max_duration_secs, auto_stop_secs, append);
    ReturnValue->Val->Integer = rc; 
}

void Sdl_audio_play_tones (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int         time_units_ms = Param[0]->Val->Integer;
    sdl_tone_t *tones         = Param[1]->Val->Pointer;
    int         rc;

    rc = sdl_audio_play_tones(time_units_ms, tones);
    ReturnValue->Val->Integer = rc; 
}

void Sdl_audio_ctl (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int req = Param[0]->Val->Integer;

    sdl_audio_ctl(req);
}

void Sdl_audio_state (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_audio_state_t *state = Param[0]->Val->Pointer;

    sdl_audio_state(state);
}

void Sdl_audio_print_device_info (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdl_audio_print_devices_info();
}

void Sdl_audio_create_test_file (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *filename      = Param[0]->Val->Pointer;
    int   duration_secs = Param[1]->Val->Integer;
    int   freq          = Param[2]->Val->Integer;

    sdl_audio_create_test_file(filename, duration_secs, freq);
}

//
// sensors
//

void Sdl_sensor_get_info_tbl (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int *num_sensors = Param[0]->Val->Pointer;
    sdl_sensor_info_t *sit;

    sit = sdl_sensor_get_info_tbl(num_sensors);
    ReturnValue->Val->Pointer = sit;
}

void Sdl_sensor_find (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int type = Param[0]->Val->Integer;
    int id;

    id = sdl_sensor_find(type);
    ReturnValue->Val->Integer = id;
}

void Sdl_sensor_read_raw (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int     id         = Param[0]->Val->Integer;
    double *data       = Param[1]->Val->Pointer;
    int     num_values = Param[2]->Val->Integer;
    int     rc;

    rc = sdl_sensor_read_raw(id, data, num_values);
    ReturnValue->Val->Integer = rc;
}

void Sdl_sensor_read_step_counter (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    unsigned long *step_count = Param[0]->Val->Pointer;
    int            rc;

    rc = sdl_sensor_read_step_counter(step_count);
    ReturnValue->Val->Integer = rc;
}

void Sdl_sensor_read_mag_heading (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    double *mag_heading = Param[0]->Val->Pointer;
    int     rc;

    rc = sdl_sensor_read_mag_heading(mag_heading);
    ReturnValue->Val->Integer = rc;
}

void Sdl_sensor_read_tilt (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    double *roll = Param[0]->Val->Pointer;
    double *pitch = Param[1]->Val->Pointer;
    int     rc;

    rc = sdl_sensor_read_tilt(roll, pitch);
    ReturnValue->Val->Integer = rc;
}

void Sdl_sensor_read_pressure (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    double *millibars = Param[0]->Val->Pointer;
    int     rc;

    rc = sdl_sensor_read_pressure(millibars);
    ReturnValue->Val->Integer = rc;
}

void Sdl_sensor_read_temperature (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    double *degrees_c = Param[0]->Val->Pointer;
    int     rc;

    rc = sdl_sensor_read_temperature(degrees_c);
    ReturnValue->Val->Integer = rc;
}

void Sdl_sensor_read_humidity (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    double *percent = Param[0]->Val->Pointer;
    int     rc;

    rc = sdl_sensor_read_humidity(percent);
    ReturnValue->Val->Integer = rc;
}

//
// SDL REGISTRATION
//

char stop_requested[100]; //xxx should be extern here
void SdlSetupFunction(Picoc *pc)
{
    #define PLATFORM_VAR(name, type, writeable) \
        do { \
            VariableDefinePlatformVar(pc, NULL, #name, &pc->type, \
                                      (union AnyValue *)&name, writeable); \
        } while (0)
        
    PLATFORM_VAR(sdl_win_width, IntType, false);
    PLATFORM_VAR(sdl_win_height, IntType, false);
    PLATFORM_VAR(sdl_char_width, IntType, false);
    PLATFORM_VAR(sdl_char_height, IntType, false);

    VariableDefinePlatformVar(pc, NULL, "stop_requested", pc->CharArrayType, 
                              (union AnyValue*)stop_requested, true);
}

struct LibraryFunction SdlFunctions[] = {
    // sdl initialization and termination, must be done once
    { Sdl_init,            "int sdl_init(void);" },
    { Sdl_exit,            "void sdl_exit(void);" },

    // display init and present, must be done for every display update
    { Sdl_display_init,    "void sdl_display_init(int color);" },
    { Sdl_display_present, "void sdl_display_present(void);" },

    // event registration and query
    { Sdl_register_event,  "void sdl_register_event(sdl_loc_t *loc, int event_id);" },
    { Sdl_register_control_events, 
                           "void sdl_register_control_events(char *evstr1, char *evstr2, char *evstr3, int bg_color, int evid1, int evid2, int evid3); " },
    { Sdl_get_event,       "void sdl_get_event(long timeout_us, sdl_event_t *event);" },

    // create colors
    { Sdl_create_color,    "int sdl_create_color(int r, int g, int b, int a);" },
    { Sdl_scale_color,     "int sdl_scale_color(int color, double inten);" },
    { Sdl_wavelength_to_color, "int sdl_wavelength_to_color(int wavelength);" },

    // render text
    { Sdl_print_init,              "void sdl_print_init(double numchars, int fg_color, int bg_color);" },
    { Sdl_print_init_color,        "void sdl_print_init_color(int fg_color, int bg_color);" },
    { Sdl_print_save,              "void sdl_print_save(sdl_print_state_t *save);" },
    { Sdl_print_restore,           "void sdl_print_restore(sdl_print_state_t *restore);" },
    { Sdl_render_text,             "sdl_loc_t *sdl_render_text(int x, int y, char *str);" },
    { Sdl_render_printf,           "sdl_loc_t *sdl_render_printf(int x, int y, char *fmt, ...);" },
    { Sdl_render_text_xyctr,       "sdl_loc_t *sdl_render_text_xyctr(int x, int y, char *str);" },
    { Sdl_render_printf_xyctr,     "sdl_loc_t *sdl_render_printf_xyctr(int x, int y, char *fmt, ...);" },
    { Sdl_render_multiline_text,   "void sdl_render_multiline_text(int y_top, int y_display_begin, int y_display_end, char * str);" },
    { Sdl_render_multiline_text_2, "void sdl_render_multiline_text_2(int y_top, int y_display_begin, int y_display_end, char **lines, int n);" },

    // render rectangle, lines, circles, points
    { Sdl_render_rect,     "void sdl_render_rect(int x, int y, int w, int h, int line_width, int color);" },
    { Sdl_render_fill_rect,"void sdl_render_fill_rect(int x, int y, int w, int h, int color);" },
    { Sdl_render_line,     "void sdl_render_line(int x1, int y1, int x2, int y2, int color);" },
    { Sdl_render_lines,    "void sdl_render_lines(sdl_point_t *points, int count, int color);" },
    { Sdl_render_circle,   "void sdl_render_circle(int x_ctr, int y_ctr, int radius, int line_width, int color);" },
    { Sdl_render_point,    "void sdl_render_point(int x, int y, int color, int point_size);" },
    { Sdl_render_points,   "void sdl_render_points(sdl_point_t *points, int count, int color, int point_size);" },

    // render using textures
    { Sdl_create_texture_from_pixels,   "sdl_texture_t *sdl_create_texture_from_pixels(sdl_pixels_t *pixels);" },
    { Sdl_create_filled_circle_texture, "sdl_texture_t *sdl_create_filled_circle_texture(int radius, int color);" },
    { Sdl_create_text_texture,          "sdl_texture_t *sdl_create_text_texture(char *str);" },
    { Sdl_render_texture,               "void sdl_render_texture(int x, int y, int w, int h, double angle, sdl_texture_t *texture);" },
    { Sdl_destroy_texture,              "void sdl_destroy_texture(sdl_texture_t *texture);" },
    { Sdl_query_texture,                "void sdl_query_texture(sdl_texture_t *texture, int *width, int *height);" },
    { Sdl_read_display_pixels,          "void *sdl_read_display_pixels(int x, int y, int w, int h);" },

    // audio
    { Sdl_audio_play,                   "int sdl_audio_play(char *filename);" },
    { Sdl_audio_record,                 "int sdl_audio_record(char *filename, int max_duration_secs, int auto_stop_secs, bool append); "},
    { Sdl_audio_play_tones,             "int sdl_audio_play_tones(int time_units_ms, sdl_tone_t *tones);" },
    { Sdl_audio_ctl,                    "void sdl_audio_ctl(int req);" },
    { Sdl_audio_state,                  "void sdl_audio_state(sdl_audio_state_t * state);" },
    { Sdl_audio_print_device_info,      "void sdl_audio_print_devices_info(void);" },
    { Sdl_audio_create_test_file,       "void sdl_audio_create_test_file(char *filename, int duration_secs, int freq);" },

    // sensors
    { Sdl_sensor_get_info_tbl,          "sdl_sensor_info_t *sdl_sensor_get_info_tbl(int *num_sensors);" },
    { Sdl_sensor_find,                  "int sdl_sensor_find(int type);" },
    { Sdl_sensor_read_raw,              "int sdl_sensor_read_raw(int id, double *data, int num_values);" },
    { Sdl_sensor_read_step_counter,     "int sdl_sensor_read_step_counter(unsigned long *step_count);" },
    { Sdl_sensor_read_mag_heading,      "int sdl_sensor_read_mag_heading(double *mag_heading);" },
    { Sdl_sensor_read_tilt,             "int sdl_sensor_read_tilt(double *roll, double *pitch);" },
    { Sdl_sensor_read_pressure,         "int sdl_sensor_read_pressure(double *millibars);" },
    { Sdl_sensor_read_temperature,      "int sdl_sensor_read_temperature(double *degrees_c);" },
    { Sdl_sensor_read_humidity,         "int sdl_sensor_read_humidity(double *percent);" },

    { NULL, NULL } };

const char SdlDefs[] = "\
typedef struct sdl_texture sdl_texture_t; \n\
typedef struct { \n\
    int x; \n\
    int y; \n\
    int w; \n\
    int h; \n\
} sdl_loc_t; \n\
typedef struct { \n\
    int x; \n\
    int y; \n\
} sdl_point_t; \n\
typedef struct { \n\
    int event_id; \n\
    union { \n\
        struct { \n\
            double x; double y; double xrel; double yrel; \n\
        } motion; \n\
    } u; \n\
} sdl_event_t; \n\
typedef struct { \n\
    int magic; \n\
    int struct_len; \n\
    int w; \n\
    int h; \n\
    int pixels[0]; \n\
} sdl_pixels_t; \n\
typedef struct { \n\
    short freq; \n\
    short intvl; \n\
} sdl_tone_t; \n\
typedef struct { \n\
    int  state; \n\
    bool paused; \n\
    int  processed_secs; \n\
    int  total_secs; \n\
    int  volume; \n\
    char filename[100]; \n\
} sdl_audio_state_t; \n\
typedef struct { \n\
    int   id; \n\
    int   type; \n\
    char *name; \n\
} sdl_sensor_info_t; \n\
typedef struct { \n\
    int ptsize; \n\
    int char_width; \n\
    int char_height; \n\
    int bg_color; \n\
    int fg_color; \n\
} sdl_print_state_t; \n\
\n\
#define PIXELS_MAGIC 0x11223344 \n\
\n\
#define BYTES_PER_PIXEL  4 \n\
#define COLOR_BLACK      (   0  |    0<<8 |    0<<16 |  255<<24 ) \n\
#define COLOR_WHITE      ( 255  |  255<<8 |  255<<16 |  255<<24 ) \n\
#define COLOR_RED        ( 255  |    0<<8 |    0<<16 |  255<<24 ) \n\
#define COLOR_ORANGE     ( 255  |  128<<8 |    0<<16 |  255<<24 ) \n\
#define COLOR_YELLOW     ( 255  |  255<<8 |    0<<16 |  255<<24 ) \n\
#define COLOR_GREEN      (   0  |  255<<8 |    0<<16 |  255<<24 ) \n\
#define COLOR_BLUE       (   0  |    0<<8 |  255<<16 |  255<<24 ) \n\
#define COLOR_INDIGO     (  75  |    0<<8 |  130<<16 |  255<<24 ) \n\
#define COLOR_VIOLET     ( 238  |  130<<8 |  238<<16 |  255<<24 ) \n\
#define COLOR_PURPLE     ( 127  |    0<<8 |  255<<16 |  255<<24 ) \n\
#define COLOR_LIGHT_BLUE (   0  |  255<<8 |  255<<16 |  255<<24 ) \n\
#define COLOR_PINK       ( 255  |  105<<8 |  180<<16 |  255<<24 ) \n\
#define COLOR_TEAL       (   0  |  128<<8 |  128<<16 |  255<<24 ) \n\
#define COLOR_LIGHT_GRAY ( 192  |  192<<8 |  192<<16 |  255<<24 ) \n\
#define COLOR_GRAY       ( 128  |  128<<8 |  128<<16 |  255<<24 ) \n\
#define COLOR_DARK_GRAY  (  64  |   64<<8 |   64<<16 |  255<<24 ) \n\
\n\
#define ROW2Y(r) ((r) * sdl_char_height) \n\
#define COL2X(c) ((c) * sdl_char_width) \n\
\n\
#define EVID_SWIPE_RIGHT       9990 \n\
#define EVID_SWIPE_LEFT        9991 \n\
#define EVID_MOTION            9992 \n\
#define EVID_QUIT              9999 \n\
\n\
#define AUDIO_STATE_IDLE          0 \n\
#define AUDIO_STATE_PLAY_FILE     1 \n\
#define AUDIO_STATE_PLAY_TONES    2 \n\
#define AUDIO_STATE_RECORD        3 \n\
#define AUDIO_STATE_RECORD_APPEND 4 \n\
\n\
#define AUDIO_REQ_STOP     1 \n\
#define AUDIO_REQ_PAUSE    2 \n\
#define AUDIO_REQ_UNPAUSE  3 \n\
\n\
#define ASENSOR_TYPE_ACCELEROMETER       1 \n\
#define ASENSOR_TYPE_MAGNETIC_FIELD      2 \n\
#define ASENSOR_TYPE_GYROSCOPE           4 \n\
#define ASENSOR_TYPE_LIGHT               5 \n\
#define ASENSOR_TYPE_PRESSURE            6 \n\
#define ASENSOR_TYPE_PROXIMITY           8 \n\
#define ASENSOR_TYPE_GRAVITY             9 \n\
#define ASENSOR_TYPE_LINEAR_ACCELERATION 10 \n\
#define ASENSOR_TYPE_ROTATION_VECTOR     11 \n\
#define ASENSOR_TYPE_RELATIVE_HUMIDITY   12 \n\
#define ASENSOR_TYPE_AMBIENT_TEMPERATURE 13 \n\
#define ASENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED 14 \n\
#define ASENSOR_TYPE_GAME_ROTATION_VECTOR 15 \n\
#define ASENSOR_TYPE_GYROSCOPE_UNCALIBRATED 16 \n\
#define ASENSOR_TYPE_SIGNIFICANT_MOTION 17 \n\
#define ASENSOR_TYPE_STEP_DETECTOR 18 \n\
#define ASENSOR_TYPE_STEP_COUNTER 19 \n\
#define ASENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR 20 \n\
#define ASENSOR_TYPE_HEART_RATE 21 \n\
#define ASENSOR_TYPE_POSE_6DOF 28 \n\
#define ASENSOR_TYPE_STATIONARY_DETECT 29 \n\
#define ASENSOR_TYPE_MOTION_DETECT 30 \n\
#define ASENSOR_TYPE_HEART_BEAT 31 \n\
#define ASENSOR_TYPE_DYNAMIC_SENSOR_META 32 \n\
#define ASENSOR_TYPE_ADDITIONAL_INFO 33 \n\
#define ASENSOR_TYPE_LOW_LATENCY_OFFBODY_DETECT 34 \n\
#define ASENSOR_TYPE_ACCELEROMETER_UNCALIBRATED 35 \n\
#define ASENSOR_TYPE_HINGE_ANGLE 36 \n\
#define ASENSOR_TYPE_HEAD_TRACKER 37 \n\
#define ASENSOR_TYPE_ACCELEROMETER_LIMITED_AXES 38 \n\
#define ASENSOR_TYPE_GYROSCOPE_LIMITED_AXES 39 \n\
#define ASENSOR_TYPE_ACCELEROMETER_LIMITED_AXES_UNCALIBRATED 40 \n\
#define ASENSOR_TYPE_GYROSCOPE_LIMITED_AXES_UNCALIBRATED 41 \n\
#define ASENSOR_TYPE_HEADING 42 \n\
";

// -----------------  UTILS PLATFORM ROUTINES  --------------------------

//
// utils time routines
//

void Util_microsec_timer (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    ReturnValue->Val->LongInteger = util_microsec_timer();
}

void Util_get_real_time_microsec (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    ReturnValue->Val->LongInteger = util_get_real_time_microsec();
}

void Util_time2str (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *str          = Param[0]->Val->Pointer;
    long  us           = Param[1]->Val->LongInteger;
    int   gmt          = Param[2]->Val->Integer;
    int   display_ms   = Param[3]->Val->Integer;
    int   display_date = Param[4]->Val->Integer;
    char *s;

    s = util_time2str(str, us, gmt, display_ms, display_date);
    ReturnValue->Val->Pointer = s;
}

//
// utils file write / read routines
//

void Util_write_file (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *dir  = Param[0]->Val->Pointer;
    char *fn   = Param[1]->Val->Pointer;
    void *data = Param[2]->Val->Pointer;
    int   len  = Param[3]->Val->Integer;
    int   ret;

    ret = util_write_file(dir, fn, data, len);
    ReturnValue->Val->Integer = ret;
}

void Util_read_file (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *dir  = Param[0]->Val->Pointer;
    char *fn   = Param[1]->Val->Pointer;
    int  *len  = Param[2]->Val->Pointer;
    void *file_contents;

    file_contents = util_read_file(dir, fn, len);
    ReturnValue->Val->Pointer = file_contents;
}

//
// utils params
//

void Util_get_str_param(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    char *dir           = Param[0]->Val->Pointer;
    char *name          = Param[1]->Val->Pointer;
    char *default_value = Param[2]->Val->Pointer;
    char *value;

    value = util_get_str_param(dir, name, default_value);
    ReturnValue->Val->Pointer = value;
}

void Util_set_str_param(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    char *dir   = Param[0]->Val->Pointer;
    char *name  = Param[1]->Val->Pointer;
    char *value = Param[2]->Val->Pointer;

    util_set_str_param(dir, name, value);
}

void Util_get_int_param(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    char *dir           = Param[0]->Val->Pointer;
    char *name          = Param[1]->Val->Pointer;
    int   default_value = Param[2]->Val->Integer;
    int   value;

    value = util_get_int_param(dir, name, default_value);
    ReturnValue->Val->Integer = value;
}

void Util_set_int_param(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    char *dir   = Param[0]->Val->Pointer;
    char *name  = Param[1]->Val->Pointer;
    int   value = Param[2]->Val->Integer;

    util_set_int_param(dir, name, value);
}

void Util_print_params(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    char *dir = Param[0]->Val->Pointer;

    util_print_params(dir);
}

//
// utils network
//

void Util_get_ipaddr(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    char *ipaddr;

    ipaddr = util_get_ipaddr();
    ReturnValue->Val->Pointer = ipaddr;
}

//
// UTILS REGISTRATION
//

void UtilsSetupFunction(Picoc *pc)
{
}

struct LibraryFunction UtilsFunctions[] = {
    // time
    { Util_microsec_timer,   "long util_microsec_timer(void);" },
    { Util_get_real_time_microsec, "long util_get_real_time_microsec(void);" },
    { Util_time2str,         "char *util_time2str(char * str, long us, bool gmt, bool display_ms, bool display_date);" },
    // file read/write
    { Util_write_file,       "int util_write_file(char *dir, char *fn, void *data, int len);" },
    { Util_read_file,        "void *util_read_file(char *dir, char *fn, int *len);" },
    // params get/set
    { Util_get_str_param,    "char *util_get_str_param(char *dir, char *name, char *default_value);" },
    { Util_set_str_param,    "void util_set_str_param(char *dir, char *name, char *value);" },
    { Util_get_int_param,    "int util_get_int_param(char *dir, char *name, int default_value);" },
    { Util_set_int_param,    "void util_set_int_param(char *dir, char *name, int value);" },
    { Util_print_params,     "void util_print_params(char *dir);" },
    // network
    { Util_get_ipaddr,       "char *util_get_ipaddr(void);" },

    { NULL, NULL } };

const char UtilsDefs[] = "";

// -----------------  PLATFORM INIT PROC  -------------------------------

void PlatformLibraryInit(Picoc *pc)
{
    IncludeRegister(
        pc, 
        "sdl.h", 
        SdlSetupFunction,
        SdlFunctions, 
        SdlDefs);

    IncludeRegister(
        pc, 
        "utils.h", 
        UtilsSetupFunction,
        UtilsFunctions, 
        UtilsDefs);
}
