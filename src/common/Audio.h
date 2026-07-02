#pragma once
#include <string>

// 音訊管理:背景音樂播放 / 靜音切換 / 音量。SDL_mixer 後端。
// 初始化失敗(無音訊裝置)時所有呼叫皆 no-op,不影響遊戲。
class Audio {
public:
    static void init();
    static void shutdown();

    // 載入並循環播放背景音樂(ogg/mp3/wav)。失敗回傳 false。
    // quiet=true:缺檔屬預期(如版權平台曲未隨附),不印 error,交由呼叫端 fallback。
    static bool playMusic(const std::string &path, bool quiet = false);

    // 切換音樂靜音(暫停/恢復)。回傳切換後是否「開啟」。
    static bool toggleMute();

    // 播放一次性音效(獨立於音樂的 channel;chunk 依路徑快取)。音效靜音時為 no-op。
    static void playSfx(const std::string &path);
    // 切換音效靜音(獨立於音樂)。回傳切換後是否「開啟」。
    static bool toggleSfx();

    static bool isMuted() { return _muted; }
    static bool sfxMuted() { return _sfxMuted; }
    static bool available() { return _ok; }

private:
    static bool _ok;     // SDL_mixer 是否成功初始化
    static bool _muted;
    static bool _sfxMuted;
    static bool _hasMusic;
};
