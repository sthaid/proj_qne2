#include <stdio.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>

int main()
{
    printf("SDL_Version %d\n", SDL_GetVersion());
    printf("MIX_Version %d\n", MIX_Version());
    printf("TTF_Version %d\n", TTF_Version());
}
