#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <iostream>
#include <string>

int main(int argc, char* args[]) {
    // 1. Initialize SDL
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // 2. Open the audio device (SDL3_mixer handles the internal mixer creation)
    // You can specify properties here if needed, but NULL uses defaults
    SDL_AudioDeviceID deviceId = SDL_OpenAudioDevice(NULL, NULL);
    if (!deviceId) {
        std::cerr << "Failed to open audio device! SDL Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // 3. Load the MP3 music file
    // Use the correct path to your MP3 file
    std::string musicPath = "music.mp3"; 
    MIX_Audio* music = MIX_LoadAudio(musicPath.c_str(), NULL);
    if (!music) {
        std::cerr << "Failed to load music! SDL_mixer Error: " << MIX_GetError() << std::endl;
        SDL_CloseAudioDevice(deviceId);
        SDL_Quit();
        return 1;
    }

    // 4. Create a track and play the music
    // The second parameter to MIX_CreateTrack is a properties object for the track, use NULL for defaults
    MIX_Track* track = MIX_CreateTrack(NULL);
    if (!track) {
        std::cerr << "Failed to create track! SDL_mixer Error: " << MIX_GetError() << std::endl;
        MIX_DestroyAudio(music);
        SDL_CloseAudioDevice(deviceId);
        SDL_Quit();
        return 1;
    }

    // Play the loaded audio on the track
    if (MIX_PlayTrack(track, music, -1) == -1) { // -1 means loop indefinitely
        std::cerr << "Failed to play music! SDL_mixer Error: " << MIX_GetError() << std::endl;
        MIX_DestroyTrack(track); // Destroy track if it fails to play
        MIX_DestroyAudio(music);
        SDL_CloseAudioDevice(deviceId);
        SDL_Quit();
        return 1;
    }

    std::cout << "Playing music... Press Enter to stop." << std::endl;
    std::cin.get(); // Wait for user input to keep the program running

    // 5. Clean up resources
    MIX_HaltTrack(track); // Stop the track
    MIX_DestroyTrack(track); // Free the track
    MIX_DestroyAudio(music); // Free the audio data
    SDL_CloseAudioDevice(deviceId); // Close the device
    MIX_Quit(); // Quit the mixer subsystem
    SDL_Quit(); // Quit SDL subsystems

    return 0;
}

