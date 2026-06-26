#pragma once
#include <string>

// 音訊管理:背景音樂播放 / 靜音切換 / 音量。SDL_mixer 後端。
// 初始化失敗(無音訊裝置)時所有呼叫皆 no-op,不影響遊戲。
class Audio {
public:
    static void init();
    static void shutdown();

    // 載入並循環播放背景音樂(ogg/mp3/wav)。失敗則靜默忽略。
    static void playMusic(const std::string &path);

    // 切換靜音(暫停/恢復音樂)。回傳切換後是否「開啟」。
    static bool toggleMute();

    static bool isMuted() { return _muted; }
    static bool available() { return _ok; }

private:
    static bool _ok;     // SDL_mixer 是否成功初始化
    static bool _muted;
    static bool _hasMusic;
};
