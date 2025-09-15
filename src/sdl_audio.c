#include <std_hdrs.h>

#include <sdl.h>
#include <utils.h>
#include <logging.h>

#include <SDL3/SDL.h>

//
// defines
//

#define TEN_MS 10000

//
// defines
//

#define FRAMES_PER_SEC 48000

#define BYTES_TO_SECS(b) nearbyint(((double)(b) / 2 / FRAMES_PER_SEC))

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
static int calc_volume(void *buff, int bytes);

static int play_file_thread(void *cx);
static void play_buff(char *buff, int buff_len, bool *stop_req, int *queued_bytes);
static int record_thread(void *cx);
static int tones_thread(void *cx);

// -----------------INIT / EXIT  -------------------------------

int sdl_audio_init(void)
{
    INFO("initializing\n");

    // initialize SDL audio
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        ERROR("SDL_Init AUDIO failed, %s\n", SDL_GetError());
        return -1;
    }

    // success
    INFO("success\n");
    return 0;
}

void sdl_audio_quit(void)
{
    INFO("quitting\n");

    // quit SDL audio
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

// -----------------  OPEN AUDIO FOR PLAYBACK OR RECORD --------------------

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
    SDL_AudioDeviceID *devid;
    int i, count;
    const char *name;

    devid = SDL_GetAudioPlaybackDevices(&count);
    INFO("num playback devices = %d\n", count);
    for (i = 0; i < count; i++) {
        name = SDL_GetAudioDeviceName(devid[i]);
        INFO("  playback dev %d = %s\n", devid[i], name);
    }

    devid = SDL_GetAudioRecordingDevices(&count);
    INFO("num recording devices = %d\n", count);
    for (i = 0; i < count; i++) {
        name = SDL_GetAudioDeviceName(devid[i]);
        INFO("  recording dev %d = %s\n", devid[i], name);
    }
}

void sdl_audio_create_test_file(char *dir, char *filename, int duration_secs, int freq)
{
    int    frames = duration_secs * FRAMES_PER_SEC;
    int    n = FRAMES_PER_SEC / freq;
    int    i, fd;
    short *buff;
    char   path[100];

    // allocate and init buffer, that will be written to the test file
    buff = malloc(frames*2);
    for (i = 0; i < frames; i++) {
        buff[i] = 32767 * sin((2*M_PI) * ((double)i / n));
    }

    // write the buffer to test file
    sprintf(path, "%s/%s", dir, filename);
    fd = open(path, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if (fd < 0) {
        ERROR("failed to create '%s', %s\n", path, strerror(errno));
        return;
    }
    write(fd, buff, frames*2);
    close(fd);

    // free buffer
    free(buff);
}

static int calc_volume(void *buff, int bytes)
{
    short *samples = (short*)buff;
    int    n = bytes/2;
    int    sum = 0;
    int    average;

    // calculate average of the absolute value of the samples;
    for (int i = 0; i < n; i++) {
        sum += abs(samples[i]);
    }
    average = nearbyint((double)sum / n * (156. / 32768.));
    if (average > 100) average = 100;

    // publish volume, and return
    state.volume = average;
    return average;
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

int sdl_audio_play(char *dir, char *filename)
{
    int rc, fd=-1;
    void *buff=MAP_FAILED;
    struct stat statbuf;
    play_file_cx_t *cx=NULL;
    char path[100];

    // open audio for playback
    rc = audio_open(PLAYBACK);
    if (rc < 0) {
        ERROR("failed to open audio for playback\n");
        goto error;
    }

    // obtain size of file, and map it
    sprintf(path, "%s/%s", dir, filename);
    rc = stat(path, &statbuf);
    if (rc < 0) {
        ERROR("failed to stat '%s', %s\n", path, strerror(errno));
        goto error;
    }
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        ERROR("failed to open '%s', %s\n", path, strerror(errno));
        goto error;
    }
    buff = mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    fd = -1;
    if (buff == MAP_FAILED) {
        ERROR("failed to map '%s', %s\n", path, strerror(errno));
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
    sdl_create_detached_thread(play_file_thread, cx);

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

static int play_file_thread(void *cx_arg)
{
    play_file_cx_t *cx = (play_file_cx_t*)cx_arg;
    int queued_bytes = 0;
    bool stop_req = false;

    INFO("starting\n");

    // resume audio playback
    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(playback_stream));  // thread safe
    state.paused = false;

    // play the buffer
    play_buff(cx->buff, cx->buff_len, &stop_req, &queued_bytes);  // thread safe
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
    return 0;
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
        SDL_PutAudioStreamData(playback_stream, buff_ptr, xfer_len);  // thread safe

        // calculate volume for the samples just queued
        calc_volume(buff_ptr, xfer_len);

        // while there is more than 200 ms queued OR do_once
        // - process control requests
        // - publish amount played
        // - short sleep
        // note SDL_GetAudioStreamQueued is thread safe
        do_once = true;
        while ((SDL_GetAudioStreamQueued(playback_stream) > FRAMES_PER_SEC / 5 * sizeof(short)) || (do_once)) {
            // process control requests
            if (ctl_req == AUDIO_REQ_STOP) {
                *stop_req = true;
                ctl_req = 0;
                return;
            } else if (ctl_req == AUDIO_REQ_PAUSE) {
                SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(playback_stream));  // thread safe
                state.paused = true;
                ctl_req = 0;
            } else if (ctl_req == AUDIO_REQ_UNPAUSE) {
                SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(playback_stream));  // thread safe
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
    int  total_secs;
    int  auto_stop_secs;
    int  existing_bytes;
} record_cx_t;

int sdl_audio_record(char *dir, char *filename, int max_duration_secs, int auto_stop_secs, bool append)
{
    int rc, fd=-1;
    record_cx_t *cx=NULL;
    int existing_bytes;
    struct stat statbuf;
    char path[100];

    // open audio to record
    rc = audio_open(RECORD);
    if (rc < 0) {
        ERROR("failed to open audio for record\n");
        goto error;
    }

    // if not appending then
    //   create new recording file
    // else
    //   open existing recording file, in append mode
    //   determine the size of the existing file
    // endif
    sprintf(path, "%s/%s", dir, filename);
    if (!append) {
        fd = open(path, O_WRONLY|O_CREAT|O_TRUNC, 0666);
        if (fd < 0) {
            ERROR("failed to create '%s', %s\n", path, strerror(errno));
            goto error;
        }
        existing_bytes = 0;
    } else {
        fd = open(path, O_WRONLY|O_APPEND);
        if (fd < 0) {
            ERROR("failed to open for append '%s', %s\n", path, strerror(errno));
            goto error;
        }
        fstat(fd, &statbuf);
        existing_bytes = statbuf.st_size;
    }

    // init state
    memset(&state, 0, sizeof(state));
    state.state          = (!append ? AUDIO_STATE_RECORD : AUDIO_STATE_RECORD_APPEND);
    state.paused         = true;
    state.total_secs     = max_duration_secs + BYTES_TO_SECS(existing_bytes);
    strcpy(state.filename, filename);

    // create thread to xfer the record data to a file
    cx = malloc(sizeof(record_cx_t));
    cx->fd              = fd;
    cx->total_secs      = state.total_secs; 
    cx->auto_stop_secs  = auto_stop_secs;
    cx->existing_bytes  = existing_bytes;
    sdl_create_detached_thread(record_thread, cx);

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

static int record_thread(void *cx_arg)
{
    record_cx_t *cx = (record_cx_t*)cx_arg;
    short        buff[4096];
    int          rc, bytes, volume, silence_bytes = 0;
    int          processed_bytes = 0;

    const int silence_volume = 5;
    const int auto_stop_bytes = cx->auto_stop_secs * FRAMES_PER_SEC * 2;

    INFO("starting\n");

    // start recording
    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(record_stream));  // thread safe
    state.paused = false;

    while (true) {
        // get audio data, if non available then short sleep and try again
        bytes = SDL_GetAudioStreamData(record_stream, buff, sizeof(buff));  // thread safe
        if (bytes == -1) {
            ERROR("SDL_GetAudioStreamData failed, %s\n", SDL_GetError());
            break;
        }
        if (bytes == 0) {
            usleep(TEN_MS);
            continue;
        }

        // write the data to the file
        rc = write(cx->fd, buff, bytes);
        if (rc != bytes) {
            ERROR("write failed, rc=%d, %s\n", rc, strerror(errno));
            break;
        }

        // keep track of how long the recording has been in progress
        processed_bytes += bytes;
        state.processed_secs = BYTES_TO_SECS(processed_bytes + cx->existing_bytes);

        // calculate volume of the samples just obtained
        volume = calc_volume(buff, bytes);
        printf("volume %d\n", volume);

        // if auto_stop is enabled then if silent for n secs stop recording
        if (cx->auto_stop_secs > 0) {
            if (volume < silence_volume) {
                silence_bytes += bytes;
            } else {
                silence_bytes = 0;
            }
            if (silence_bytes > auto_stop_bytes) {
                break;
            }
        }

        // if have captured frames for the desired time interval then break
        if (state.processed_secs >= cx->total_secs) {
            break;
        }

        // handle control requests
        if (ctl_req == AUDIO_REQ_STOP) {
            ctl_req = 0;
            break;
        } else if (ctl_req == AUDIO_REQ_PAUSE) {
            SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(record_stream));  // thread safe
            state.paused = true;
            ctl_req = 0;
        } else if (ctl_req == AUDIO_REQ_UNPAUSE) {
            SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(record_stream));  // thread safe
            state.paused = false;
            ctl_req = 0;
        }

        // short sleep
        usleep(TEN_MS);
    }

    // pause and clear the record stream
    SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(record_stream));  // thread safe
    SDL_ClearAudioStream(record_stream);  // thread safe

    // add short tone to end of file, this is a delimiter when appending
    // to an existing recording
    static short *tone;
    #define AMPLITUDE      5000     // max 32767
    #define HZ             500
    #define NUM_SIN_WAVES  (HZ / 4)  // 1/4 sec
    #define N_ONE_SIN_WAV  (FRAMES_PER_SEC / HZ)   // samples in 1 sine wave
    #define N_TOTAL        (NUM_SIN_WAVES * N_ONE_SIN_WAV)  // total samples
    if (tone == NULL) {
        tone = malloc(2 * N_TOTAL);
        for (int j = 0; j < N_TOTAL; j++) {
            tone[j] = AMPLITUDE * sin((2*M_PI) * ((double)j / N_ONE_SIN_WAV));
        }
    }
    write(cx->fd, tone, 2 * N_TOTAL);

    // cleanup and return
    INFO("completed\n");
    close(cx->fd);
    free(cx);
    memset(&state, 0, sizeof(state));
    return 0;
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
    state.total_secs     = nearbyint(duration_ms / 1000.);
    strcpy(state.filename, "");

    // create thread to play the tones
    cx = malloc(sizeof(play_tones_cx_t) + num_tones * sizeof(sdl_tone_t));
    cx->time_units_ms  = time_units_ms;
    cx->num_tones = num_tones;
    memcpy(cx->tones, tones, num_tones * sizeof(sdl_tone_t));
    sdl_create_detached_thread(tones_thread, cx);

    // success
    return 0;
}

static int tones_thread(void *cx_arg)
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
            n = nearbyint(FRAMES_PER_SEC / t->freq);
            sw = malloc(sizeof(int) + n * sizeof(short));
            sw->n = n;
            for (j = 0; j < n; j++) {
                sw->data[j] = 32767 * sin((2*M_PI) * ((double)j / n));
            }
            sine_waves[t->freq] = sw;
        }
    }

    // start playing tones
    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(playback_stream));  // thread safe
    state.paused = false;

    // loop over the tones
    for (int i = 0; i < cx->num_tones; i++) {
        sdl_tone_t *t = &cx->tones[i];
        int tone_intvl_ms = t->intvl * cx->time_units_ms;
        int len;

        INFO("tone[%d] freq=%d millisecs=%d\n", i, t->freq, tone_intvl_ms);

        // construct buff for either:
        // - gap  (when t->freq == 0), or
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
        play_buff(buff, len, &stop_req, &queued_bytes);  // thread safe
        if (stop_req) {
            goto done;
        }
    }

    // wait for all queued audio to be played
    while (SDL_GetAudioStreamQueued(playback_stream) > 0) {  // thread safe
        usleep(TEN_MS);
    }

done:
    // cleanup and return
    INFO("completed\n");
    SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(playback_stream));  // thread safe
    SDL_ClearAudioStream(playback_stream);  // thread safe
    free(cx);
    free(buff);
    memset(&state, 0, sizeof(state));
    return 0;
}
