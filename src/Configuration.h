#pragma once

#include <nlohmann/json.hpp>
#include <string>

using namespace std;

class Configuration {
public:
    static void init();

    static int getScreenWidth() {
        return _value.at("resolution_width").get<int>();
    }

    static int getScreenHeight() {
        return _value.at("resolution_height").get<int>();
    }

    static string getGameFilesPath() {
        return _value.at("game_files_path").get<string>();
    }

    static string getLanguage() {
        return _value.value("language", string("zh-Hant"));
    }

    // 時間流速 %(每步 time_accum += speed_pct,滿 100 走一 tick;100=每步一 tick=原行為)
    // 執行期可由 F6 設定選單調整(初值取自 config.json)
    static int getSpeedPct() { return _speedPct; }
    static void setSpeedPct(int v) { _speedPct = clampPct(v); }

    // 怪物生成率 %(每步生成機率倍率;參考 u2-cht)
    static int getSpawnPct() { return _spawnPct; }
    static void setSpawnPct(int v) { _spawnPct = clampPct(v); }

    // 食物消耗速度 %(每時間 tick 扣食物量倍率;100=每 tick −1=原行為,越高越快餓)
    static int getFoodPct() { return _foodPct; }
    static void setFoodPct(int v) { _foodPct = clampPct(v); }

    // 野外怪物是否自動追蹤玩家。預設關(忠於 1981「出現即攻擊、不移動」);
    // 開 = 每時間 tick 朝玩家走一步(貪婪逐玩家,參考 u2-cht)。
    static bool getChaseMonsters() { return _chaseMonsters; }
    static void setChaseMonsters(bool v) { _chaseMonsters = v; }

    static string getEgaOverworldTilesFilePath() {
        return getGameFilesPath() + "EGATILES.BIN";
    }

    static string getCgaOverworldTilesFilePath() {
        return getGameFilesPath() + "CGATILES.BIN";
    }

    // Tandy 1000(16 色,與 EGA 同 RowPlanar 格式;T1KTILES.BIN 與 EGATILES.BIN 同大小)
    static string getTandyOverworldTilesFilePath() {
        return getGameFilesPath() + "T1KTILES.BIN";
    }

    // 初始 tileset:"ega" / "cga" / "png"(AssetPack:png 用 PNG sprite sheet 包)
    static string getTileset() {
        return _value.value("tileset", string("ega"));
    }

    // PNG AssetPack 路徑(跨平台素材包;tileset="png" 或循環到 PNG 時用)
    static string getTilesetPng() {
        return _value.value("tileset_png", string("assets/tilesets/ega.png"));
    }

    static string getEgaTownFilePath() {
        return getGameFilesPath() + "EGATOWN.BIN";
    }

    static string getCgaTownFilePath() {
        return getGameFilesPath() + "CGATOWN.BIN";
    }

    static string getMapFilePath() {
        return getGameFilesPath() + "MAP.BIN";
    }

    static string getTownMapFilePath() {
        return getGameFilesPath() + "TCD.BIN";
    }

private:
    static nlohmann::json _value;
    static int _speedPct;   // 執行期值(init 時取自 config)
    static int _spawnPct;
    static int _foodPct;
    static bool _chaseMonsters;
    static int clampPct(int v) { return v < 10 ? 10 : (v > 200 ? 200 : v); }
};
