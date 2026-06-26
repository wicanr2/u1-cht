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
    static int getSpeedPct() {
        return _value.value("speed_pct", 100);
    }

    // 怪物生成率 %(每步生成機率倍率;參考 u2-cht)
    static int getSpawnPct() {
        return _value.value("spawn_pct", 55);
    }

    static string getEgaOverworldTilesFilePath() {
        return getGameFilesPath() + "EGATILES.BIN";
    }

    static string getCgaOverworldTilesFilePath() {
        return getGameFilesPath() + "CGATILES.BIN";
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
};
