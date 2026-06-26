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
