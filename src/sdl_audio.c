#include <std_hdrs.h>
#include <math.h>

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
// typedefs
//

//
// variables
//

//
// prototypes
//

// ----------------- xxxxxxxxxxx --------------------------

#if 0
SDL_AudioStream   advanced

SDL_InitSubSystem(SDL_INIT_AUDIO
   or
Mix_Init
Mix_OpenAudio

SDL_OpenAudioDevice
SDL_OpenAudioDeviceStream    simple
If you want to queue audio, you can leave the callback null and use SDL_QueueAudio

Use SDL_QueueAudio to provide audio data to the device. SDL will manage the playback of this data. 

SDL_PauseAudioDevice

SDL_CloseAudioDevice

If using SDL_AudioStream, you can control its behavior with functions like SDL_AudioStreamPut and SDL_AudioStreamGet

------------

SDL_GetAudioDeviceName


SDL_AudioDeviceID SDL_OpenAudioDevice(
                          const char *device,
                          int iscapture,
                          const SDL_AudioSpec *desired,
                          SDL_AudioSpec *obtained,
                          int allowed_changes);

SDL_CloseAudioDevice
SDL_GetAudioDeviceName
SDL_LockAudioDevice
SDL_OpenAudio
SDL_PauseAudioDevice
SDL_UnlockAudioDevice



xxx MORE xxx
SDL_ClearQueuedAudio

#endif

#define PLAYBACK 0
#define CAPTURE  1

#define PAUSE_OFF 0
#define PAUSE_ON  1

void print_sdl_audio_spec(char *hdr, SDL_AudioSpec *spec);
void playback_cb(void *userdata, unsigned char *buff, int buff_len_bytes);
void play_tone(double duration, double hz);

SDL_AudioDeviceID device_id;

void sdl_audio(void)
{
    int i, num;
    int rc;
    char *name;
    SDL_AudioSpec spec;
    SDL_AudioSpec desired, obtained;

    num = SDL_GetNumAudioDevices(PLAYBACK);
    printf("playback devices: num=%d\n", num);
    for (i = 0; i < num; i++) {
        printf("  %d: %s\n", i, SDL_GetAudioDeviceName(i, PLAYBACK));
    }

    num = SDL_GetNumAudioDevices(CAPTURE);
    printf("recording devices: num=%d\n", num);
    for (i = 0; i < num; i++) {
        printf("  %d: %s\n", i, SDL_GetAudioDeviceName(i, CAPTURE));
    }

    memset(&spec, 0, sizeof(spec));
    name = NULL;
    rc = SDL_GetDefaultAudioInfo(&name, &spec, PLAYBACK);
    printf("SDL_GetDefaultAudioInfo rc=%d\n", rc);
    if (rc == 0) {
        printf("  name = '%s'\n", name);
        print_sdl_audio_spec("default spec", &spec);
    }

    memset(&desired, 0, sizeof(desired));
    memset(&obtained, 0, sizeof(obtained));

    desired.freq     = 48000;  // xxx ?
    desired.format   = AUDIO_S16SYS;
    desired.channels = 1;
    desired.silence  = 0; // calculated
    desired.samples  = 4096;
    desired.size     = 0; // calculated
    //xxx desired.callback = playback_cb;
    desired.userdata = NULL;

    // SDL_AUDIO_ALLOW_FREQUENCY_CHANGE
    // little endian
    device_id = SDL_OpenAudioDevice(
                NULL,  // request default device
                PLAYBACK,
                &desired,
                &obtained,
                SDL_AUDIO_ALLOW_ANY_CHANGE);  //xxx
    if (device_id == 0) {
        printf("ERROR: SDL_OpenAudioDevice failed, %s\n", SDL_GetError());
        return;
    }
    printf("device_id = %d\n", device_id);
    print_sdl_audio_spec("desired", &desired);
    print_sdl_audio_spec("obtained", &obtained);

    SDL_PauseAudioDevice(device_id, PAUSE_OFF);

    for (i = 0; i < 5; i++) {
        play_tone(1, 1000);
        sleep(1);
    }
}

void play_tone(double duration, double hz)
{
    int i, samples, rc, queued_bytes;
    static short *buff;
    static bool first_call = true;
    long start, dur;

    samples = duration * 48000;

    if (first_call) {
        printf("init buff\n");
        start = util_microsec_timer();
        buff = malloc(2 * samples);
        for (i = 0; i < samples; i++) {
            buff[i] =  10000 * sin( (2*M_PI) * (i / 48.) );
        }
        dur = util_microsec_timer() - start;
        printf("done init buff dur=%ld ms\n", dur/1000);
        first_call = false;
    }

    rc = SDL_QueueAudio(device_id, buff, 2*samples);
    printf("SDL_QueueAudio ret %d\n", rc);


    while (true) {
        queued_bytes =  SDL_GetQueuedAudioSize(device_id);
        if (queued_bytes == 0) {
            break;
        }
        usleep(10000); // xxx use SDL sleep ?
    }
}

void playback_cb(void *userdata, unsigned char *buff_arg, int len_arg)
{
    short *buff = (short*)buff_arg;
    int    len = len_arg / 2;
    int i;

    static bool first_call = true;
    static short sine_wave[48];
    static int j;

    // len is in bytes
    printf("playback_cb called len_arg=%d\n", len_arg);

    if (first_call) {
        for (i = 0; i < 48; i++) {
            sine_wave[i] = 10000 * sin( (2*M_PI) * (i / 48.) );  // xxx nearbyint
        }
        first_call = false;
    }

    for (i = 0; i < len; i++) {
        buff[i] = sine_wave[j];
        j = j + 1;
        if (j == 48) j = 0;
    }
}

void print_sdl_audio_spec(char *hdr, SDL_AudioSpec *spec)
{
    #define BIT15 0x8000
    #define BIT12 0x1000
    #define BIT8  0x0100
    printf("%s\n", hdr);
    printf("  freq     = %d\n", spec->freq);
    printf("  format   = 0x%x\n", spec->format);
    printf("             %s\n", ((spec->format & BIT15) ? "signed" : "unsigned"));
    printf("             %s\n", ((spec->format & BIT12) ? "big_endian" : "little_endian"));
    printf("             %s\n", ((spec->format & BIT8) ? "float" : "integer"));
    printf("             %d bits\n", spec->format & 0xff);
    printf("  channels = %d\n", spec->channels);
    printf("  silence  = %d\n", spec->silence);
    printf("  samples  = %d\n", spec->samples);
    printf("  size     = %d\n", spec->size);
    printf("  callback = %p\n", spec->callback);
    printf("  userdata = %p\n", spec->userdata);
}
