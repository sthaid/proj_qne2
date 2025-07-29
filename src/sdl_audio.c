#include <std_hdrs.h>

#include <sdl.h>
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


#endif

#define PLAYBACK 0
#define CAPTURE  1

void sdl_audio(void)
{
    int i, num;
    int rc;
    char *name;
    SDL_AudioSpec spec;

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
    }

}
