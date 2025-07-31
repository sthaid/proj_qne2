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
#define CAPTURE  1

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

//
// variables
//

static SDL_AudioDeviceID device_id;
static SDL_AudioSpec     obtained;
static play_t            play;

//
// prototypes
//

static void print_sdl_audio_spec(char *hdr, SDL_AudioSpec *spec);
static int play_proc(void *buff, int buff_frames, int total_frames, bool wait);
static void play_cb(void *userdata, unsigned char *buff, int len_bytes);

// -----------------  OPEN / CLOSE  -----------------------

int sdl_audio_open(int frames_per_sec, int channels)
{
    SDL_AudioSpec desired;

    memset(&desired, 0, sizeof(desired));
    memset(&obtained, 0, sizeof(obtained));

    // init desired output format
    desired.freq     = frames_per_sec;
    desired.format   = AUDIO_S16SYS;
    desired.channels = channels;
    desired.silence  = 0;     // calculated in the obtained return
    desired.samples  = 4096;  // frames
    desired.size     = 0;     // calculated in the obtained return
    desired.callback = play_cb;
    desired.userdata = NULL;

    // open the audio device
    device_id = SDL_OpenAudioDevice(
                    NULL,  // request default device
                    PLAYBACK,
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
    num = SDL_GetNumAudioDevices(CAPTURE);
    INFO("recording devices: num=%d\n", num);
    for (i = 0; i < num; i++) {
        INFO("  %d: %s\n", i, SDL_GetAudioDeviceName(i, CAPTURE));
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

static int play_proc(void *buff, int buff_frames, int total_frames, bool wait)
{
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

    play_proc(buff, n, total_frames, false);
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
    play_proc(buff, total_frames, total_frames, false);

    // xxx need to unmap
}


void sdl_audio_wait(void)
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

