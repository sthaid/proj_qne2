#include <std_hdrs.h>

#include <math.h>
#include <sys/mman.h>

#include <sdl.h>
#include <utils.h>
#include <logging.h>

// xxx todo
// - state should include PAUSED status     <=====
// - what is best place to unpause    <=====
// - add timeouts to the threads    <=====
// - add timeouts to usleep loops
// - test pause and unpause of al modes
// - record auto stop
// - search for usleep loops that could have a timeout
// - add FRAME_SIZE macro
// - 30000 amplitude ?
// - why is record not reliable
// - record append option

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

#define PLAYBACK false
#define RECORD   true

#define PAUSE_OFF 0
#define PAUSE_ON  1

#define FRAMES_PER_SEC 48000

#ifdef ANDROID
    #define DEVICE NULL  // request default device
#else
    #define DEVICE (record ? "USB PnP Audio Device Mono" : NULL)
#endif

#define BYTES_TO_SECS(b) ((b) / 2 / FRAMES_PER_SEC)

//
// typedefs
//

//
// variables
//

static SDL_AudioDeviceID device_id;
static bool              recording;
static SDL_AudioSpec     obtained;
static int               ctl_req;
static sdl_audio_state_t state;

//
// prototypes
//

static int audio_open(bool record);
static void print_sdl_audio_spec(char *hdr, SDL_AudioSpec *spec);
static void *play_thread(void *cx);
static void *record_thread(void *cx);
static void *tones_thread(void *cx);

void calc_volume(void *buff, int bytes);

// -----------------  OPEN / CLOSE  -----------------------

static int audio_open(bool record)
{
    SDL_AudioSpec desired;

    INFO("called to %s\n", record ? "RECORD" : "PLAYBACK");

    // if audio is active then stop it
    sdl_audio_ctl(AUDIO_REQ_STOP);

    // if audio is already open, and in the correct mode, then return
    if (device_id > 0 && recording == record) {
        INFO("already open\n");
        return 0;
    }

    // need to reopen
    SDL_CloseAudioDevice(device_id);
    device_id = 0;

    // init the obtained format to zero
    memset(&obtained, 0, sizeof(obtained));

    // init desired format
    memset(&desired, 0, sizeof(desired));
    desired.freq     = FRAMES_PER_SEC;
    desired.format   = AUDIO_S16SYS;
    desired.channels = 1;
    desired.silence  = 0;     // calculated in the obtained return
    desired.samples  = 4096;  // frames
    desired.size     = 0;     // calculated in the obtained return
    desired.callback = NULL;
    desired.userdata = NULL;

    // open the audio device
    device_id = SDL_OpenAudioDevice(
                    DEVICE,
                    record,
                    &desired,
                    &obtained,
                    //0);    // no changes allowd
                    SDL_AUDIO_ALLOW_ANY_CHANGE);
    if (device_id == 0) {
        ERROR("SDL_OpenAudioDevice failed, %s\n", SDL_GetError());
        return -1;
    }
    recording = record;
    INFO("device_id = %d recording = %d\n", device_id, recording);

    // print the obtained output format
    print_sdl_audio_spec("obtained", &obtained);

    // return success
    return 0;
}

static void print_sdl_audio_spec(char *hdr, SDL_AudioSpec *spec)
{
    #define BIT15 0x8000
    #define BIT12 0x1000
    #define BIT8  0x0100

    INFO("%s\n", hdr);
    INFO("  freq     = %d\n", spec->freq);
    INFO("  format   = 0x%x\n", spec->format);
    INFO("             %s\n", ((spec->format & BIT15) ? "signed" : "unsigned"));
    INFO("             %s\n", ((spec->format & BIT12) ? "big_endian" : "little_endian"));
    INFO("             %s\n", ((spec->format & BIT8) ? "float" : "integer"));
    INFO("             %d bits\n", spec->format & 0xff);
    INFO("  channels = %d\n", spec->channels);
    INFO("  silence  = %d\n", spec->silence);
    INFO("  samples  = %d\n", spec->samples);
    INFO("  size     = %d\n", spec->size);
    INFO("  callback = %p\n", spec->callback);
    INFO("  userdata = %p\n", spec->userdata);
}

// -----------------  DEBUG & SUPPORT ---------------------

void sdl_audio_print_devices_info(void)
{
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
        buff[i] = 30000 * sin((2*M_PI) * ((double)i / n));
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
    int not_used;
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

    // queue playback 
    rc = SDL_QueueAudio(device_id, buff, statbuf.st_size);
    if (rc < 0) {
        ERROR("failed to queue playback, rc=%d, %s\n", rc, SDL_GetError());
        goto error;
    }
    INFO("QUEUED %d\n", SDL_GetQueuedAudioSize(device_id));

    // unmap
    munmap(buff, statbuf.st_size);
    buff = MAP_FAILED;

    // init state
    memset(&state, 0, sizeof(state));
    state.state          = AUDIO_STATE_PLAY_FILE;
    state.paused         = true;
    state.total_secs     = BYTES_TO_SECS(statbuf.st_size);
    strcpy(state.filename, filename);

    // create thread to monitor and process completion
    cx = malloc(sizeof(play_file_cx_t));
    cx->not_used = 0;
    pthread_create(&tid, NULL, play_thread, cx);

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

static void *play_thread(void *cx_arg)
{
    int bytes_remaining;
    play_file_cx_t *cx = (play_file_cx_t*)cx_arg;

    INFO("starting\n");

    // unpause
    SDL_PauseAudioDevice(device_id, PAUSE_OFF);
    state.paused = false;

    // wait for play to complete, and process control requests
    while (true) {
        usleep(TEN_MS);

        bytes_remaining = SDL_GetQueuedAudioSize(device_id);
        state.processed_secs = state.total_secs - BYTES_TO_SECS(bytes_remaining);

        if (bytes_remaining == 0) {
            break;
        }
        
        if (ctl_req == AUDIO_REQ_STOP) {
            ctl_req = 0;
            break;
        } else if (ctl_req == AUDIO_REQ_PAUSE) {
            SDL_PauseAudioDevice(device_id, PAUSE_ON);
            state.paused = true;
            ctl_req = 0;
        } else if (ctl_req == AUDIO_REQ_UNPAUSE) {
            SDL_PauseAudioDevice(device_id, PAUSE_OFF);
            state.paused = false;
            ctl_req = 0;
        }
    }

    // cleanup and return
    INFO("completed\n");
    SDL_PauseAudioDevice(device_id, PAUSE_ON);
    SDL_ClearQueuedAudio(device_id);
    free(cx);
    memset(&state, 0, sizeof(state));
    return NULL;
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

    // unpause
    SDL_PauseAudioDevice(device_id, PAUSE_OFF);
    state.paused = false;

    while (true) {
        // get audio data, if non available then short sleep and try again
        bytes = SDL_DequeueAudio(device_id, buff, sizeof(buff));
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

        // xxx volume
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
            SDL_PauseAudioDevice(device_id, PAUSE_ON);
            state.paused = true;
            ctl_req = 0;
        } else if (ctl_req == AUDIO_REQ_UNPAUSE) {
            SDL_PauseAudioDevice(device_id, PAUSE_OFF);
            state.paused = false;
            ctl_req = 0;
        }

        // short sleep
        usleep(TEN_MS);
    }

    // cleanup and return
    INFO("completed\n");
    SDL_PauseAudioDevice(device_id, PAUSE_ON);
    SDL_ClearQueuedAudio(device_id);
    close(cx->fd);
    free(cx);
    memset(&state, 0, sizeof(state));
    return NULL;
}

// -----------------  PLAY TONE  --------------------------

typedef struct {
    int n;
    short data[0];
} sine_wave_t;

typedef struct {
    int time_units_ms;
    int num_tones;
    tone_t tones[0];
} play_tones_cx_t;

#define MIN_TONE_FREQ 20
#define MAX_TONE_FREQ 5000

// xxx malloc this ?
static sine_wave_t *sine_waves[MAX_TONE_FREQ+1];

int sdl_audio_play_tones(int time_units_ms, tone_t *tones)
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

    // loop over tones arg to determine total duration and num_tones
    // xxx comment
    num_tones = 0;
    duration_ms = 0;
    for (i = 0; tones[i].intvl > 0; i++) {
        tone_t *t = &tones[i];
        duration_ms += (t->intvl * time_units_ms);
        num_tones++;
    }

    // init state
    memset(&state, 0, sizeof(state));
    state.state          = AUDIO_STATE_PLAY_TONES; 
    state.paused         = true;
    state.total_secs     = duration_ms / 1000 + 1;
    strcpy(state.filename, "");

    // create thread to play the tones
    cx = malloc(sizeof(play_tones_cx_t) + num_tones * sizeof(tone_t));
    cx->time_units_ms  = time_units_ms;
    cx->num_tones = num_tones;
    memcpy(cx->tones, tones, num_tones * sizeof(tone_t));
    pthread_create(&tid, NULL, tones_thread, cx);

    // success
    return 0;
}

static void *tones_thread(void *cx_arg)
{
    play_tones_cx_t *cx = (play_tones_cx_t*)cx_arg;
    char *buff = NULL;
    int queued_bytes = 0;
    int buff_len;
    bool do_once;

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
        tone_t *t = &cx->tones[i];
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
            INFO("creating sine wave at freq %d  n=%d\n", t->freq, n);
            for (j = 0; j < n; j++) {
                sw->data[j] = 30000 * sin((2*M_PI) * ((double)j / n));
            }
            sine_waves[t->freq] = sw;
        }
    }

    // unpause
    SDL_PauseAudioDevice(device_id, PAUSE_OFF);
    state.paused = false;

    // xxx comment
    for (int i = 0; i < cx->num_tones; i++) {
        tone_t *t = &cx->tones[i];
        int tone_intvl_ms = t->intvl * cx->time_units_ms;
        int len;

        INFO("tone[%d] freq=%d millisecs=%d\n", i, t->freq, tone_intvl_ms);

        // xxx comment
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

        // queue the gap or tone
        SDL_QueueAudio(device_id, buff, len);
        queued_bytes += len;

        // while there is more than 100 ms queued OR do_once 
        // - process control requests
        // - publish amount played
        // - short sleep
        do_once = true;
        while ((SDL_GetQueuedAudioSize(device_id) > FRAMES_PER_SEC / 10 * sizeof(short)) ||
               (do_once))
        {
            // process control requests
            if (ctl_req == AUDIO_REQ_STOP) {
                ctl_req = 0;
                goto done;
            } else if (ctl_req == AUDIO_REQ_PAUSE) {
                SDL_PauseAudioDevice(device_id, PAUSE_ON);
                state.paused = true;
                ctl_req = 0;
            } else if (ctl_req == AUDIO_REQ_UNPAUSE) {
                SDL_PauseAudioDevice(device_id, PAUSE_OFF);
                state.paused = false;
                ctl_req = 0;
            }

            // publish amount played
            state.processed_secs = BYTES_TO_SECS(queued_bytes - SDL_GetQueuedAudioSize(device_id));

            // short sleep
            usleep(TEN_MS);

            // clear flag that ensures this code block is executed at leas once
            do_once = false;
        }
    }

    // wait for all queued audio to be played
    while (SDL_GetQueuedAudioSize(device_id) > 0) {
        usleep(TEN_MS);
    }

done:
    // cleanup and return
    INFO("completed\n");
    SDL_PauseAudioDevice(device_id, PAUSE_ON);
    SDL_ClearQueuedAudio(device_id);
    free(cx);
    free(buff);
    memset(&state, 0, sizeof(state));
    return NULL;
}

// xxxxxxxxxxxxxxxxxxxxxxxx

// xxx update display every 50 ms

void calc_volume(void *buff, int bytes)
{
    short *samples = (short*)buff;
    int    n = bytes/2;
    int    sum = 0;
    int    average;

    // discard if number of samples not 4096
    if (n != 4096) {
        INFO("discarding n=%d\n", n);
        return;
    }

    // calculate average of the absolute value of the samples;
    // note: duration = 4096 / 48000 = 85 ms
    for (int i = 0; i < n; i++) {
        sum += abs(samples[i]);
    }
    average = nearbyint((double)sum / n * (100. / 32768.));

    // publish volume
    state.volume = average;
}
