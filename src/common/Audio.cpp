#include "Audio.h"
#include <SDL.h>
#include <SDL_mixer.h>
#include <cstdio>

bool Audio::_ok = false;
bool Audio::_muted = false;
bool Audio::_hasMusic = false;

static Mix_Music *gMusic = nullptr;

void Audio::init() {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        printf("[Audio] no audio subsystem: %s\n", SDL_GetError());
        return;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("[Audio] Mix_OpenAudio failed: %s\n", Mix_GetError());
        return;
    }
    _ok = true;
    Mix_VolumeMusic(MIX_MAX_VOLUME / 2);
    printf("[Audio] initialized\n");
}

void Audio::playMusic(const std::string &path) {
    if (!_ok) return;
    gMusic = Mix_LoadMUS(path.c_str());
    if (!gMusic) {
        printf("[Audio] Mix_LoadMUS failed (%s): %s\n", path.c_str(), Mix_GetError());
        return;
    }
    _hasMusic = true;
    Mix_PlayMusic(gMusic, -1);  // -1 = 無限循環
    printf("[Audio] playing music: %s\n", path.c_str());
}

bool Audio::toggleMute() {
    if (!_ok || !_hasMusic) return false;
    _muted = !_muted;
    if (_muted) Mix_PauseMusic();
    else Mix_ResumeMusic();
    return !_muted;
}

void Audio::shutdown() {
    if (gMusic) { Mix_FreeMusic(gMusic); gMusic = nullptr; }
    if (_ok) { Mix_CloseAudio(); }
}
