#pragma once

#include <string>
#include "Player.h"

// 存檔系統:把玩家狀態 + F6 設定序列化成 JSON。
// 離開鐵則(esc-cancel-f10-quit-autosave):F10「Y 離開」前 autosave;啟動時若有存檔則載入。
class SaveGame {
public:
    // 預設存檔路徑(目前用 cwd 的 save.json;打包後改用使用者可寫目錄)。
    static std::string defaultPath();

    // 存檔:成功回傳 true(寫檔失敗回傳 false,呼叫端不可默默吞掉)。
    static bool save(const Player &player, const std::string &path);

    // 載入:檔案存在且格式正確才套用到 player + 設定,回傳 true;否則 false(維持新遊戲預設)。
    static bool load(Player &player, const std::string &path);

    static bool exists(const std::string &path);
};
