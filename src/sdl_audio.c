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

#define FRAMES_PER_SEC 48000

//
// typedefs
//

//
// variables
//

static SDL_AudioDeviceID device_id;
static SDL_AudioSpec     obtained;

//
// prototypes
//

static int audio_open(int record);
static void audio_close(void);
static void print_sdl_audio_spec(char *hdr, SDL_AudioSpec *spec);
static void *play_thread(void *cx);

// -----------------  OPEN / CLOSE  -----------------------

static int audio_open(int record)
{
    SDL_AudioSpec desired;

    INFO("called to %s\n", record ? "RECORD" : "PLAYBACK");

    memset(&desired, 0, sizeof(desired));
    memset(&obtained, 0, sizeof(obtained));

    // init desired format
    desired.freq     = FRAMES_PER_SEC;
    desired.format   = AUDIO_S16SYS;
    desired.channels = (record ? 1 : 2);
    desired.silence  = 0;     // calculated in the obtained return
    desired.samples  = 4096;  // frames
    desired.size     = 0;     // calculated in the obtained return
    desired.callback = NULL;
    desired.userdata = NULL;

    // open the audio device
    device_id = SDL_OpenAudioDevice(
#ifdef ANDROID  // make a macro xxx
                    NULL,  // request default device
#else
                    record ? "USB PnP Audio Device Mono" : NULL,
#endif
                    record ? RECORD : PLAYBACK,
                    &desired,
                    &obtained,
                    //0);    // no changes allowd
                    SDL_AUDIO_ALLOW_ANY_CHANGE);
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

static void audio_close(void)
{
    if (device_id == 0) {
        return;
    }

    SDL_CloseAudioDevice(device_id);
    device_id = 0;
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

// -----------------  DEBUG & SUPPORT ---------------------

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

// xxx this can return status
void sdl_audio_create_test_file(void)
{
    int    duration_ms = 10000;
    int    freq        = 1000;
    char  *filename    = "audio_test";

    int    frames = duration_ms * FRAMES_PER_SEC / 1000;
    int    n = FRAMES_PER_SEC / freq;
    int    i, j, fd;
    short *buff, value;

    // allocate and init buffer, that will be written to the test file
    buff = malloc(frames*4);
    for (i = 0, j = 0; i < frames; i++) {
        value = 30000 * sin((2*M_PI) * ((double)i / n));
        buff[j++] = value;      // left
        buff[j++] = value;      // right 
    }

    // write the buffer to test file
    fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    write(fd, buff, frames*4);
    close(fd);

    // free buffer0
    free(buff);
}

// -----------------  PLAY  -------------------------------

int sdl_audio_play(char *filename)
{
    int rc, fd=-1;
    void *buff=MAP_FAILED;
    struct stat statbuf;
    pthread_t tid;

    // if busy then return error
    if (device_id > 0) {
        ERROR("audio is inuse\n");
        return -1;
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

    // open audio for playback
    rc = audio_open(PLAYBACK);
    if (rc < 0) {
        ERROR("failed to open audio for playback\n");
        goto error;
    }

    // queue playback 
    rc = SDL_QueueAudio(device_id, buff, statbuf.st_size);
    if (rc < 0) {
        ERROR("failed to queue playback, rc=%d, %s\n", rc, SDL_GetError());
        goto error;
    }
    INFO("QUEUED %d\n", SDL_GetQueuedAudioSize(device_id));

    // unpause
    SDL_PauseAudioDevice(device_id, PAUSE_OFF);

    // unmap
    munmap(buff, statbuf.st_size);
    buff = MAP_FAILED;

    // create thread to wait for completion, upon completion the thread will
    // - close audio
    pthread_create(&tid, NULL, play_thread, NULL);

    // success
    return 0;

error:
    // error cleanup and return
    audio_close();
    if (buff != MAP_FAILED) {
        munmap(buff, statbuf.st_size);
    }
    if (fd >= 0) {
        close(fd);
    }
    return -1;
}

static void *play_thread(void *cx)
{
    INFO("starting\n");

    while (SDL_GetQueuedAudioSize(device_id) > 0) {
        usleep(10000);  // 10 ms
    }
    INFO("play completed\n");

    audio_close();
    return NULL;
}

