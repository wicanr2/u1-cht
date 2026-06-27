#include "Audio.h"
#include <SDL.h>
#include <SDL_mixer.h>
#include <cstdio>
#include <string>
#include <map>

bool Audio::_ok = false;
bool Audio::_muted = false;
bool Audio::_sfxMuted = false;
bool Audio::_hasMusic = false;

static Mix_Music *gMusic = nullptr;
static std::string gMusicPath;
static std::map<std::string, Mix_Chunk *> gChunks;   // 音效 chunk 快取(依路徑)

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
    Mix_AllocateChannels(8);   // 音效用 channel(獨立於音樂)
    printf("[Audio] initialized\n");
}

bool Audio::playMusic(const std::string &path) {
    if (!_ok) return false;
    // 同一首已在播 → 不重載(避免切回同平台時音樂從頭跳)。
    if (_hasMusic && path == gMusicPath) return true;
    Mix_Music *next = Mix_LoadMUS(path.c_str());
    if (!next) {
        printf("[Audio] Mix_LoadMUS failed (%s): %s\n", path.c_str(), Mix_GetError());
        return false;
    }
    if (gMusic) { Mix_HaltMusic(); Mix_FreeMusic(gMusic); }  // 釋放舊曲,避免 leak
    gMusic = next;
    gMusicPath = path;
    _hasMusic = true;
    Mix_PlayMusic(gMusic, -1);  // -1 = 無限循環
    if (_muted) Mix_PauseMusic();  // 維持使用者既有靜音狀態
    printf("[Audio] playing music: %s\n", path.c_str());
    return true;
}

void Audio::playSfx(const std::string &path) {
    if (!_ok || _sfxMuted) return;
    auto it = gChunks.find(path);
    Mix_Chunk *c = (it != gChunks.end()) ? it->second : nullptr;
    if (!c) {
        c = Mix_LoadWAV(path.c_str());   // 缺檔 → 靜默忽略(c==nullptr 也快取,避免重複嘗試)
        gChunks[path] = c;
    }
    if (c) Mix_PlayChannel(-1, c, 0);
}

bool Audio::toggleSfx() {
    _sfxMuted = !_sfxMuted;
    if (_sfxMuted) Mix_HaltChannel(-1);   // 立即停掉正在播的音效
    return !_sfxMuted;
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
    for (auto &kv : gChunks) if (kv.second) Mix_FreeChunk(kv.second);
    gChunks.clear();
    if (_ok) { Mix_CloseAudio(); }
}
