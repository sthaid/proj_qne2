#include <std_hdrs.h>
#include <math.h>
#include <sys/mman.h>

#include <sdl.h>
#include <utils.h>
#include <logging.h>

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

//
// defines
//

#define PLAYBACK 0
#define RECORD   1

#define PAUSE_OFF 0
#define PAUSE_ON  1
//
// typedefs
//

typedef struct play_t {
    bool   playing;
    short *buff_short;
    int   *buff_int;
    int    buff_frames;
    int    total_frames;
    int    idx;
    int    played;
} play_t;

typedef struct record_t {
    bool   recording;
    short *buff_short;
    int   *buff_int;
    int    total_frames;
    int    recorded;
} record_t;

//
// variables
//

static SDL_AudioDeviceID device_id;
static SDL_AudioSpec     obtained;
static play_t            play;
static record_t          record;

//
// prototypes
//

static void print_sdl_audio_spec(char *hdr, SDL_AudioSpec *spec);
static void play_cb(void *userdata, unsigned char *buff, int len_bytes);
static void record_cb(void *userdata, unsigned char *buff, int len_bytes);

// -----------------  OPEN / CLOSE  -----------------------

int sdl_audio_open(int frames_per_sec, int channels, bool record)
{
    SDL_AudioSpec desired;

    INFO("called, frames_per_sec=%d channels=%d record=%d\n", frames_per_sec, channels, record); 

    memset(&desired, 0, sizeof(desired));
    memset(&obtained, 0, sizeof(obtained));

    // init desired output format
    desired.freq     = frames_per_sec;
    desired.format   = AUDIO_S16SYS;
    desired.channels = channels;
    desired.silence  = 0;     // calculated in the obtained return
    desired.samples  = 4096;  // frames
    desired.size     = 0;     // calculated in the obtained return
    desired.callback = record ? record_cb : play_cb;
    desired.userdata = NULL;

    // open the audio device
    device_id = SDL_OpenAudioDevice(
#ifdef ANDROID
                    NULL,  // request default device
#else
                    record ? "USB PnP Audio Device Mono" : NULL,
#endif
                    record ? RECORD : PLAYBACK,
                    &desired,
                    &obtained,
                    0);    // no changes allowd
    if (device_id == 0) {
        ERROR("SDL_OpenAudioDevice failed, %s\n", SDL_GetError());
        return -1;
    }
    INFO("device_id = %d\n", device_id);

    // print the obtained output format
    print_sdl_audio_spec("obtained", &obtained);

    // return success
    return 0;
}

void sdl_audio_close(void)
{
    if (device_id > 0) {
        SDL_CloseAudioDevice(device_id);
        device_id = 0;
        memset(&play, 0, sizeof(play));
    }
}

void sdl_audio_print_devices_info(void)
{
    int num, i;

    // print list of playback devices
    num = SDL_GetNumAudioDevices(PLAYBACK);
    INFO("playback devices: num=%d\n", num);
    for (i = 0; i < num; i++) {
        INFO("  %d: %s\n", i, SDL_GetAudioDeviceName(i, PLAYBACK));
    }

    // print list of capture devices
    num = SDL_GetNumAudioDevices(RECORD);
    INFO("recording devices: num=%d\n", num);
    for (i = 0; i < num; i++) {
        INFO("  %d: %s\n", i, SDL_GetAudioDeviceName(i, RECORD));
    }
}

static void print_sdl_audio_spec(char *hdr, SDL_AudioSpec *spec)
{
    #define BIT15 0x8000
    #define BIT12 0x1000
    #define BIT8  0x0100
    INFO("%s\n", hdr);
    INFO("  freq     = %d\n", spec->freq);
    INFO("  format   = 0x%x\n", spec->format);
    INFO("               %s\n", ((spec->format & BIT15) ? "signed" : "unsigned"));
    INFO("               %s\n", ((spec->format & BIT12) ? "big_endian" : "little_endian"));
    INFO("               %s\n", ((spec->format & BIT8) ? "float" : "integer"));
    INFO("               %d bits\n", spec->format & 0xff);
    INFO("  channels = %d\n", spec->channels);
    INFO("  silence  = %d\n", spec->silence);
    INFO("  samples  = %d\n", spec->samples);
    INFO("  size     = %d\n", spec->size);
    INFO("  callback = %p\n", spec->callback);
    INFO("  userdata = %p\n", spec->userdata);
}

// -----------------  PLAY  -------------------------------

void sdl_audio_play(void *buff, int buff_frames, int total_frames)
{
    // xxx use duration instead of total_frames

    // if device is not open, or play is in progress then return error
    if (device_id == 0 || play.playing == true) {
        ERROR("device either not open or busy\n");
    }

    // init play state
//xxx memset
    play.playing       = true;
    play.buff_short    = buff;
    play.buff_int      = buff;
    play.buff_frames   = buff_frames;
    play.total_frames  = total_frames;
    play.idx           = 0;
    play.played        = 0;

    // unpause
    SDL_PauseAudioDevice(device_id, PAUSE_OFF);
}

void sdl_audio_play_tone(int freq, int duration_ms)
{
    int i, n, total_frames;
    static char buff[20000];  // xxx check for overflow
    int frames_per_sec = 48000; //xxx

    n = nearbyint((double)frames_per_sec / freq);
    for (i = 0; i < n; i++) {
        ((short*)buff)[i] = 10000 * sin((2*M_PI) * ((double)i / n));  //xxx 10000
    }

    total_frames = (duration_ms / 1000.) * frames_per_sec;

    sdl_audio_play(buff, n, total_frames);
}

void sdl_audio_play_file(char *filename) 
{
    int rc, file_size, fd, total_frames;
    struct stat statbuf;
    int *buff;

    // open file, and get its size
    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        ERROR("failed to oen file, %s\n", strerror(errno));
        return;
    }

    rc = fstat(fd, &statbuf);
    if (rc < 0) {
        printf("fstat failed, %s\n", strerror(errno));
        return;
    }
    file_size = statbuf.st_size;
    printf("file_size %d\n", file_size);

    // map file contents to buffer
    buff = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (buff == MAP_FAILED) {
        printf("mmap failed, %s\n", strerror(errno));
        return;
    } else {
        printf("buff %p\n", buff);
        printf("buff[0] = %x\n", buff[0]);
    }
    //close(fd);

    total_frames = file_size / 2;
    sdl_audio_play(buff, total_frames, total_frames);

    // xxx need to unmap
}

void sdl_audio_wait(void)  // xxx make this common for play and record?
{
    while (play.playing) {
        usleep(10000);
    }
}

static void play_cb(void *userdata, unsigned char *buff, int len_bytes)
{
    play_t *x = &play;
    bool  mono = true;
    int i;

    INFO("len_bytes= %d\n", len_bytes);

    if (mono) {
        short *buff_short = (short*)buff;
        for (i = 0; i < len_bytes/sizeof(short); i++) {
            buff_short[i] = x->buff_short[x->idx];
            x->idx++;
            x->played++;
            if (x->idx == x->buff_frames) {
                x->idx = 0;
            }
            if (x->played == x->total_frames) {
                memset(&buff_short[i+1],
                       0,
                       len_bytes - ((i+1) * sizeof(short)));
                SDL_PauseAudioDevice(device_id, PAUSE_ON);
                play.playing = false;
                return;
            }
        }
        return;
    } else {
        return;
    }
}

// -----------------  RECORD  -----------------------------

void sdl_audio_record(void *buff, int buff_frames)
{
    record.recording    = true;
    record.buff_short   = buff;  //xxx just one of these
    record.buff_int     = buff;
    record.total_frames = buff_frames;
    record.recorded     = 0;

    SDL_PauseAudioDevice(device_id, PAUSE_OFF);
}

static void record_cb(void *userdata, unsigned char *buff, int len_bytes)
{
    record_t *x = &record;

    INFO("X50  len_bytes %d  total_frames=%d  recorded=%d\n", len_bytes, 
           x->total_frames, x->recorded);

    bool  mono = true;
    int i;

    if (mono) {
        short *buff_short = (short*)buff;
        for (i = 0; i < len_bytes/sizeof(short); i++) {
            x->buff_short[x->recorded] = 50*buff_short[i];  //xxx
            x->recorded++;
            if (x->recorded == x->total_frames) {
                INFO("pausing record\n");
                SDL_PauseAudioDevice(device_id, PAUSE_ON);
                INFO("after pausing record\n");
                record.recording = false;
                INFO("after2 pausing record\n");
                return;
            }
        }
        return;
    } else {
        return;
    }
}

#if 0
typedef struct record_t {
    bool   recording;
    short *buff_short;
    int   *buff_int;
    int    total_frames;
    int    recorded;
} record_t;


static int record_proc(void *buff, int buff_frames, int total_frames, bool wait)
{
    // unpause
    SDL_PauseAudioDevice(device_id, PAUSE_OFF);
#if 0
    // if device is not open, or play is in progress then return error
    if (device_id == 0 || play.playing == true) {
        ERROR("device either not open or busy\n");
        return -1;
    }

    // init play state
    play.playing       = true;
    play.buff_short    = buff;
    play.buff_int      = buff;
    play.buff_frames   = buff_frames;
    play.total_frames  = total_frames;
    play.idx           = 0;
    play.played        = 0;

    // unpause
    SDL_PauseAudioDevice(device_id, PAUSE_OFF);

    // if wait flag then poll for completion
    if (wait) {
        sdl_audio_wait();
    }

    // return success
    return 0;
#endif
}
#endif
