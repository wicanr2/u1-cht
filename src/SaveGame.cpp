#include "SaveGame.h"
#include "Configuration.h"
#include "common/CardinalPoint.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdio>

using nlohmann::json;

std::string SaveGame::defaultPath() {
    return "save.json";
}

bool SaveGame::exists(const std::string &path) {
    std::ifstream f(path);
    return f.good();
}

bool SaveGame::save(const Player &player, const std::string &path) {
    json j;
    j["version"] = 1;
    j["player"] = {
        {"overworldX", player.getOverworldX()},
        {"overworldY", player.getOverworldY()},
        {"dungeonX", player.getDungeonX()},
        {"dungeonY", player.getDungeonY()},
        {"dungeonLevel", player.getDungeonLevel()},
        {"hp", player.getHP()},
        {"food", player.getFood()},
        {"xp", player.getXP()},
        {"money", player.getMoney()},
    };
    // F6 設定一併持久化(時間流速 / 生成率 / 怪物追蹤)
    j["settings"] = {
        {"speed_pct", Configuration::getSpeedPct()},
        {"spawn_pct", Configuration::getSpawnPct()},
        {"chase_monsters", Configuration::getChaseMonsters()},
    };

    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        printf("[SaveGame] 寫檔失敗(無法開啟): %s\n", path.c_str());
        return false;
    }
    out << j.dump(2);
    if (!out.good()) {
        printf("[SaveGame] 寫檔失敗(寫入錯誤): %s\n", path.c_str());
        return false;
    }
    printf("[SaveGame] 已存檔: %s\n", path.c_str());
    return true;
}

bool SaveGame::load(Player &player, const std::string &path) {
    std::ifstream in(path);
    if (!in.good()) return false;
    json j;
    try {
        in >> j;
        auto &p = j.at("player");
        player.setOverworldX(p.at("overworldX").get<int>());
        player.setOverworldY(p.at("overworldY").get<int>());
        player.setDungeonX(p.value("dungeonX", 2));
        player.setDungeonY(p.value("dungeonY", 0));
        player.setDungeonLevel(p.value("dungeonLevel", 0));
        player.setHP(p.value("hp", 150));
        player.setFood(p.value("food", 200));
        player.setXP(p.value("xp", 0));
        player.setMoney(p.value("money", 100));
        if (j.contains("settings")) {
            auto &s = j["settings"];
            Configuration::setSpeedPct(s.value("speed_pct", Configuration::getSpeedPct()));
            Configuration::setSpawnPct(s.value("spawn_pct", Configuration::getSpawnPct()));
            Configuration::setChaseMonsters(s.value("chase_monsters", Configuration::getChaseMonsters()));
        }
    } catch (const std::exception &ex) {
        printf("[SaveGame] 存檔損毀,略過: %s (%s)\n", path.c_str(), ex.what());
        return false;
    }
    printf("[SaveGame] 已載入存檔: %s\n", path.c_str());
    return true;
}
