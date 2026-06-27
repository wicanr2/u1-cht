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
    json weapons = json::array(), armor = json::array(), spells = json::array();
    for (int i = 0; i < 16; ++i) weapons.push_back(player.getWeaponCount(i));
    for (int i = 0; i < 8; ++i) armor.push_back(player.getArmorCount(i));
    for (int i = 0; i < 8; ++i) spells.push_back(player.getSpellCount(i));
    j["player"] = {
        {"overworldX", player.getOverworldX()},
        {"overworldY", player.getOverworldY()},
        {"dungeonX", player.getDungeonX()},
        {"dungeonY", player.getDungeonY()},
        {"dungeonLevel", player.getDungeonLevel()},
        {"hp", player.getHP()},
        {"maxHp", player.getMaxHP()},
        {"food", player.getFood()},
        {"xp", player.getXP()},
        {"money", player.getMoney()},
        {"strength", player.getStrength()}, {"agility", player.getAgility()},
        {"stamina", player.getStamina()}, {"charisma", player.getCharisma()},
        {"wisdom", player.getWisdom()}, {"intelligence", player.getIntelligence()},
        {"currentWeapon", player.getCurrentWeapon()}, {"currentArmor", player.getCurrentArmor()},
        {"weapons", weapons}, {"armor", armor}, {"spells", spells},
        {"questTarget", player.getQuestTarget()}, {"questKills", player.getQuestKills()},
        {"questsCompleted", player.getQuestsCompleted()},
        {"vehicle", (int)player.getVehicle()},
        {"princessFreed", player.isPrincessFreed()},
        {"hasShuttle", player.hasShuttle()},
        {"spaceAce", player.isSpaceAce()},
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
        player.setMaxHP(p.value("maxHp", 150));
        player.setFood(p.value("food", 200));
        player.setXP(p.value("xp", 0));
        player.setMoney(p.value("money", 100));
        player.setStrength(p.value("strength", 20)); player.setAgility(p.value("agility", 20));
        player.setStamina(p.value("stamina", 20)); player.setCharisma(p.value("charisma", 20));
        player.setWisdom(p.value("wisdom", 20)); player.setIntelligence(p.value("intelligence", 20));
        player.setCurrentWeapon(p.value("currentWeapon", 0));
        player.setCurrentArmor(p.value("currentArmor", 0));
        if (p.contains("weapons")) for (int i = 0; i < 16 && i < (int)p["weapons"].size(); ++i) player.setWeaponCount(i, p["weapons"][i]);
        if (p.contains("armor")) for (int i = 0; i < 8 && i < (int)p["armor"].size(); ++i) player.setArmorCount(i, p["armor"][i]);
        if (p.contains("spells")) for (int i = 0; i < 8 && i < (int)p["spells"].size(); ++i) player.setSpellCount(i, p["spells"][i]);
        player.setQuestTarget(p.value("questTarget", 0));
        player.setQuestKills(p.value("questKills", 0));
        player.setQuestsCompleted(p.value("questsCompleted", 0));
        // 新存檔用 vehicle(int);舊存檔只有 hasRaft → 對映木筏。
        if (p.contains("vehicle"))
            player.setVehicle((Player::Vehicle)p.value("vehicle", 0));
        else if (p.value("hasRaft", false))
            player.setVehicle(Player::Vehicle::Raft);
        player.setPrincessFreed(p.value("princessFreed", false));
        player.setShuttle(p.value("hasShuttle", false));
        player.setSpaceAce(p.value("spaceAce", false));
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
