#include <std_hdrs.h>

#include <math.h>
#include <sys/mman.h>

#include <sdl.h>
#include <utils.h>
#include <logging.h>

// xxx todo
// - what is best place to unpause    <=====
// - add timeouts to the threads, and sleep loops <=======
// - test pause and unpause of al modes
// - record auto stop
// - add FRAME_SIZE macro
// - why is record not reliable
// - record append option
// - the state.processed is a little off
// - which mic was it using
// - check rets

//
// logging
//

#define INFO(fmt, args...) \
    do { \
        logmsg("INFO", __func__, fmt, ## args); \
    } while (0)
#define ERROR(fmt, args...) \
    do { \
        logmsg("ERROR", __func__, fmt, ## args); \
    } while (0)

#define TEN_MS 10000

//
// defines
//

#define FRAMES_PER_SEC 48000

#define BYTES_TO_SECS(b) ((b) / 2 / FRAMES_PER_SEC)

#define RECORD true
#define PLAYBACK false

//
// typedefs
//

//
// variables
//

static SDL_AudioStream  *playback_stream;
static SDL_AudioStream  *record_stream;
static int               ctl_req;
static sdl_audio_state_t state;

//
// prototypes
//

static int audio_open(bool record);
static void calc_volume(void *buff, int bytes);

static void *play_file_thread(void *cx);
static void play_buff(char *buff, int buff_len, bool *stop_req, int *queued_bytes);
static void *record_thread(void *cx);
static void *tones_thread(void *cx);

// -----------------  OPEN / CLOSE  -----------------------

static int audio_open(bool record)
{
    const SDL_AudioSpec spec = { SDL_AUDIO_S16, 1, FRAMES_PER_SEC };

    // if either playback or record is active then
    // it will be stopped
    sdl_audio_ctl(AUDIO_REQ_STOP);

    // open playback audio stream
    if (!record) {
        if (playback_stream != NULL) {
            return 0;
        }
        playback_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
        if (playback_stream == NULL) {
            ERROR("SDL_OpenAudioDeviceStream failed for playback\n");
            return -1;
        }
        INFO("opened playback stream\n");
        return 0;
    }

    // open record audio stream
    if (record) {
        if (record_stream != NULL) {
            return 0;
        }
        record_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_RECORDING, &spec, NULL, NULL);
        if (record_stream == NULL) {
            ERROR("SDL_OpenAudioDeviceStream failed for record\n");
            return -1;
        }
        INFO("opened recording stream\n");
        return 0;
    }

    return 0;
}

// -----------------  DEBUG & SUPPORT ROUTINES  -----------

void sdl_audio_print_devices_info(void)
{
#if 0 //xxx todo
    int num, i;

    // print list of playback devices
    num = SDL_GetNumAudioDevices(PLAYBACK);
    INFO("playback devices: num=%d\n", num);
    for (i = 0; i < num; i++) {
        INFO("  %d: %s\n", i, SDL_GetAudioDeviceName(i, false));
    }

    // print list of recording devices
    num = SDL_GetNumAudioDevices(RECORD);
    INFO("recording devices: num=%d\n", num);
    for (i = 0; i < num; i++) {
        INFO("  %d: %s\n", i, SDL_GetAudioDeviceName(i, true));
    }
#endif
}

void sdl_audio_create_test_file(char *filename, int duration_secs, int freq)
{
    int    frames = duration_secs * FRAMES_PER_SEC;
    int    n = FRAMES_PER_SEC / freq;
    int    i, fd;
    short *buff;

    // allocate and init buffer, that will be written to the test file
    buff = malloc(frames*2);
    for (i = 0; i < frames; i++) {
        buff[i] = 32767 * sin((2*M_PI) * ((double)i / n));
    }

    // write the buffer to test file
    fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if (fd < 0) {
        ERROR("failed to create '%s', %s\n", filename, strerror(errno));
        return;
    }
    write(fd, buff, frames*2);
    close(fd);

    // free buffer
    free(buff);
}

static void calc_volume(void *buff, int bytes)
{
    short *samples = (short*)buff;
    int    n = bytes/2;
    int    sum = 0;
    int    average;

    // discard if number of samples not 4096
    if (n != 4096) {
        return;
    }

    // calculate average of the absolute value of the samples;
    // note: duration = 4096 / 48000 = 85 ms
    for (int i = 0; i < n; i++) {
        sum += abs(samples[i]);
    }
    average = nearbyint((double)sum / n * (156. / 32768.));
    if (average > 100) average = 100;

    // publish volume
    state.volume = average;
}

// -----------------  CONTROL AND GET STATE  --------------

void sdl_audio_ctl(int req)
{
    if (state.state == AUDIO_STATE_IDLE) {
        ctl_req = 0;
        return;
    }

    ctl_req = req;

    while (ctl_req != 0) {
        usleep(TEN_MS);
    }
}

void sdl_audio_state(sdl_audio_state_t *x)
{
    *x = state;
}

// -----------------  PLAY FILE ---------------------------

typedef struct {
    char *buff;
    int   buff_len;
} play_file_cx_t;

int sdl_audio_play(char *filename)
{
    int rc, fd=-1;
    void *buff=MAP_FAILED;
    struct stat statbuf;
    pthread_t tid;
    play_file_cx_t *cx=NULL;

    // open audio for playback
    rc = audio_open(PLAYBACK);
    if (rc < 0) {
        ERROR("failed to open audio for playback\n");
        goto error;
    }

    // obtain size of file, and map it
    rc = stat(filename, &statbuf);
    if (rc < 0) {
        ERROR("failed to stat '%s', %s\n", filename, strerror(errno));
        goto error;
    }
    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        ERROR("failed to open '%s', %s\n", filename, strerror(errno));
        goto error;
    }
    buff = mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    fd = -1;
    if (buff == MAP_FAILED) {
        ERROR("failed to map '%s', %s\n", filename, strerror(errno));
        goto error;
    }

    // init state
    memset(&state, 0, sizeof(state));
    state.state          = AUDIO_STATE_PLAY_FILE;
    state.paused         = true;
    state.total_secs     = BYTES_TO_SECS(statbuf.st_size);
    strcpy(state.filename, filename);

    // create thread to monitor and process completion
    cx = malloc(sizeof(play_file_cx_t));
    cx->buff = buff;
    cx->buff_len = statbuf.st_size;
    pthread_create(&tid, NULL, play_file_thread, cx);

    // success
    return 0;

error:
    // error cleanup and return
    if (buff != MAP_FAILED) {
        munmap(buff, statbuf.st_size);
    }
    if (fd >= 0) {
        close(fd);
    }
    if (cx != NULL) {
        free(cx);
    }
    memset(&state, 0, sizeof(state));
    return -1;
}

static void *play_file_thread(void *cx_arg)
{
    play_file_cx_t *cx = (play_file_cx_t*)cx_arg;
    int queued_bytes = 0;
    bool stop_req = false;

    INFO("starting\n");

    // resume audio playback
    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(playback_stream));
    state.paused = false;

    // play the buffer
    play_buff(cx->buff, cx->buff_len, &stop_req, &queued_bytes);
    if (stop_req) {
        goto done;
    }

    // wait for all queued audio to be played
    while (SDL_GetAudioStreamQueued(playback_stream) > 0) {
        usleep(TEN_MS);
    }

done:
    // cleanup and return
    INFO("completed\n");
    SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(playback_stream));
    SDL_ClearAudioStream(playback_stream);
    munmap(cx->buff, cx->buff_len);
    free(cx);
    memset(&state, 0, sizeof(state));
    return NULL;
}

static void play_buff(char *buff, int buff_len, bool *stop_req, int *queued_bytes)
{
    char  *buff_ptr = buff;
    int    xfer_len, remaining;
    bool   do_once;

    *stop_req = false;

    while (true) {
        // queue 4096 samples of audio
        remaining = buff_len - (buff_ptr - buff);
        if (remaining == 0) {
            break;
        }
        xfer_len = (remaining > 8192 ? 8192 : remaining);
        SDL_PutAudioStreamData(playback_stream, buff_ptr, xfer_len);

        // calculate volume for the samples just queued
        calc_volume(buff_ptr, xfer_len);

        // while there is more than 200 ms queued OR do_once
        // - process control requests
        // - publish amount played
        // - short sleep
        do_once = true;
        while ((SDL_GetAudioStreamQueued(playback_stream) > FRAMES_PER_SEC / 5 * sizeof(short)) || (do_once)) {
            // process control requests
            if (ctl_req == AUDIO_REQ_STOP) {
                *stop_req = true;
                ctl_req = 0;
                return;
            } else if (ctl_req == AUDIO_REQ_PAUSE) {
                SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(playback_stream));
                state.paused = true;
                ctl_req = 0;
            } else if (ctl_req == AUDIO_REQ_UNPAUSE) {
                SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(playback_stream));
                state.paused = false;
                ctl_req = 0;
            }

            // publish duration played
            state.processed_secs = BYTES_TO_SECS(*queued_bytes);

            // short sleep
            usleep(TEN_MS);

            // clear flag that ensures this code block is executed at leas once
            do_once = false;
        }

        // advance to next sub-buffer
        buff_ptr += xfer_len;
        *queued_bytes += xfer_len;
    }
}

// -----------------  RECORD TO FILE ----------------------

typedef struct {
    int  fd;
    int  duration_secs;
    bool auto_stop;
} record_cx_t;

int sdl_audio_record(char *filename, int duration_secs, bool auto_stop)
{
    int rc, fd=-1;
    record_cx_t *cx=NULL;
    pthread_t tid;

    // open audio to record
    rc = audio_open(RECORD);
    if (rc < 0) {
        ERROR("failed to open audio for record\n");
        goto error;
    }

    // create empty file that will be used to store the recorded data
    fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if (fd < 0) {
        ERROR("failed to create '%s', %s\n", filename, strerror(errno));
        goto error;
    }

    // init state
    memset(&state, 0, sizeof(state));
    state.state          = AUDIO_STATE_RECORD;
    state.paused         = true;
    state.total_secs     = duration_secs;
    strcpy(state.filename, filename);

    // create thread to xfer the record data to a file
    cx = malloc(sizeof(record_cx_t));
    cx->fd            = fd;
    cx->duration_secs = duration_secs;
    cx->auto_stop     = auto_stop;
    pthread_create(&tid, NULL, record_thread, cx);

    // success
    return 0;

error:
    // error cleanup and return
    if (fd >= 0) {
        close(fd);
    }
    if (cx != NULL) {
        free(cx);
    }
    memset(&state, 0, sizeof(state));
    return -1;
}

static void *record_thread(void *cx_arg)
{
    record_cx_t *cx = (record_cx_t*)cx_arg;
    short        buff[4096];
    int          rc, bytes;
    int          processed_bytes = 0;

    INFO("starting\n");

    // start recording
    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(record_stream));
    state.paused = false;

    while (true) {
        // get audio data, if non available then short sleep and try again
        bytes = SDL_GetAudioStreamData(record_stream, buff, sizeof(buff));
        if (bytes == -1) {
            ERROR("SDL_GetAudioStreamData failed, %s\n", SDL_GetError());
            break;
        }
        if (bytes == 0) {
            usleep(TEN_MS);
            continue;
        }
        INFO("got bytes %d\n", bytes);

        // write the data to the file
        rc = write(cx->fd, buff, bytes);
        if (rc != bytes) {
            ERROR("write failed, rc=%d, %s\n", rc, strerror(errno));
            break;
        }

        // calculate volume for the samples just obtained
        calc_volume(buff, bytes);

        // keep track of how long the recording has been in progress
        processed_bytes += bytes;
        state.processed_secs = BYTES_TO_SECS(processed_bytes);

        // if have captured frames for the desired time interval then break
        if (state.processed_secs >= cx->duration_secs) {
            break;
        }

        // handle control requests
        if (ctl_req == AUDIO_REQ_STOP) {
            ctl_req = 0;
            break;
        } else if (ctl_req == AUDIO_REQ_PAUSE) {
            SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(record_stream));
            state.paused = true;
            ctl_req = 0;
        } else if (ctl_req == AUDIO_REQ_UNPAUSE) {
            SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(record_stream));
            state.paused = false;
            ctl_req = 0;
        }

        // short sleep
        usleep(TEN_MS);
    }

    // cleanup and return
    INFO("completed\n");
    SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(record_stream));
    SDL_ClearAudioStream(record_stream);
    close(cx->fd);
    free(cx);
    memset(&state, 0, sizeof(state));
    return NULL;
}

// -----------------  PLAY TONES  -------------------------

typedef struct {
    int n;
    short data[0];
} sine_wave_t;

typedef struct {
    int time_units_ms;
    int num_tones;
    sdl_tone_t tones[0];
} play_tones_cx_t;

#define MIN_TONE_FREQ 100
#define MAX_TONE_FREQ 3000 

static sine_wave_t *sine_waves[MAX_TONE_FREQ+1];

int sdl_audio_play_tones(int time_units_ms, sdl_tone_t *tones)
{
    int num_tones, duration_ms, i, rc;
    pthread_t tid;
    play_tones_cx_t *cx;

    // open audio for playback
    rc = audio_open(PLAYBACK);
    if (rc < 0) {
        ERROR("failed to open audio for playback\n");
        return -1; 
    }

    // loop over tones to determine total duration and num_tones
    num_tones = 0;
    duration_ms = 0;
    for (i = 0; tones[i].intvl > 0; i++) {
        sdl_tone_t *t = &tones[i];
        duration_ms += (t->intvl * time_units_ms);
        num_tones++;
    }

    // init state for playing tones
    memset(&state, 0, sizeof(state));
    state.state          = AUDIO_STATE_PLAY_TONES; 
    state.paused         = true;
    state.total_secs     = duration_ms / 1000 + 1;
    strcpy(state.filename, "");

    // create thread to play the tones
    cx = malloc(sizeof(play_tones_cx_t) + num_tones * sizeof(sdl_tone_t));
    cx->time_units_ms  = time_units_ms;
    cx->num_tones = num_tones;
    memcpy(cx->tones, tones, num_tones * sizeof(sdl_tone_t));
    pthread_create(&tid, NULL, tones_thread, cx);

    // success
    return 0;
}

static void *tones_thread(void *cx_arg)
{
    play_tones_cx_t *cx = (play_tones_cx_t*)cx_arg;
    int              queued_bytes = 0;
    bool             stop_req = false;

    static char     *buff;
    static int       buff_len;

    INFO("starting\n");

    // allocate buff to handle a tone or gap of up to 30 secs
    buff_len = 30 * FRAMES_PER_SEC * sizeof(short);
    buff = malloc(30 * FRAMES_PER_SEC * sizeof(short));
    INFO("XXXX allocated buff %p %d\n", buff, buff_len);
    if (buff == NULL) {
        ERROR("malloc %d failed\n", buff_len);
        goto done;
    }

    // pre calculate the sine waves for the frequency(s) requested
    for (int i = 0; i < cx->num_tones; i++) {
        sdl_tone_t *t = &cx->tones[i];
        int n, j;
        sine_wave_t *sw;

        if (t->freq != 0 && t->freq < MIN_TONE_FREQ) {
            t->freq = MIN_TONE_FREQ;
        }
        if (t->freq > MAX_TONE_FREQ) {
            t->freq = MAX_TONE_FREQ;
        }

        if (t->freq && sine_waves[t->freq] == NULL) {
            n = FRAMES_PER_SEC / t->freq;
            sw = malloc(sizeof(int) + n * sizeof(short));
            sw->n = n;
            //INFO("creating sine wave at freq %d  n=%d\n", t->freq, n);
            for (j = 0; j < n; j++) {
                sw->data[j] = 32767 * sin((2*M_PI) * ((double)j / n));
            }
            sine_waves[t->freq] = sw;
        }
    }

    // start playing tones
    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(playback_stream));
    state.paused = false;

    // loop over the tones
    for (int i = 0; i < cx->num_tones; i++) {
        sdl_tone_t *t = &cx->tones[i];
        int tone_intvl_ms = t->intvl * cx->time_units_ms;
        int len;

        INFO("tone[%d] freq=%d millisecs=%d\n", i, t->freq, tone_intvl_ms);

        // construct buff for either:
        // - gap  (when t->fre == 0), or
        // - tone
        if (t->freq == 0) {
            len = FRAMES_PER_SEC * tone_intvl_ms / 1000 * sizeof(short);
            if (len > buff_len) {
                len = buff_len;
            }
            memset(buff, 0, len);
        } else {
            sine_wave_t *sw = sine_waves[t->freq];
            int num_sine_waves = tone_intvl_ms * t->freq / 1000;
            char *buff_ptr = buff;

            if (num_sine_waves * sw->n * sizeof(short) > buff_len) {
                num_sine_waves = buff_len / (sw->n * sizeof(short));
            }

            for (int j = 0; j < num_sine_waves; j++) {
                memcpy(buff_ptr, sw->data, sw->n * sizeof(short));
                buff_ptr += sw->n * sizeof(short);
            }
            len = buff_ptr - buff;
        }

        // play the tone, or gap
        play_buff(buff, len, &stop_req, &queued_bytes);
        if (stop_req) {
            goto done;
        }
    }

    // wait for all queued audio to be played
    while (SDL_GetAudioStreamQueued(playback_stream) > 0) {
        usleep(TEN_MS);
    }

done:
    // cleanup and return
    INFO("completed\n");
    SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(playback_stream));
    SDL_ClearAudioStream(playback_stream);
    free(cx);
    free(buff);
    memset(&state, 0, sizeof(state));
    return NULL;
}
