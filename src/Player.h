#pragma once

#include "common/CardinalPoint.h"
#include "common/Direction.h"

class Player {
public:
    Player(int initialOverworldX, int initialOverworldY) : _overworldX(initialOverworldX),
                                                           _overworldY(initialOverworldY) {};

    int getOverworldX() const { return _overworldX; };

    int getOverworldY() const { return _overworldY; };

    void setOverworldX(int value) { _overworldX = value; };

    void setOverworldY(int value) { _overworldY = value; };

    int getDungeonX() const { return _dungeonX; };

    int getDungeonY() const { return _dungeonY; };

    void setDungeonX(int value) { _dungeonX = value; };

    void setDungeonY(int value) { _dungeonY = value; };

    int getDungeonLevel() const { return _dungeonLevel; };

    void setDungeonLevel(int value) { _dungeonLevel = value; };

    CardinalPoint getDungeonOrientation() { return _dungeonOrientation; };

    void dungeonTurn(Direction value);

    void dungeonMoveForward();

    int getTownX() { return _townX; }

    void setTownX(int value) { _townX = value; }

    int getTownY() { return _townY; }

    void setTownY(int value) { _townY = value; }

    int getHP() const { return _hp; };

    int getFood() const { return _food; };

    int getXP() const { return _xp; };

    int getMoney() const { return _money; };

    void receiveDamage(int damage) { _hp -= damage; };

    int getMaxHP() const { return _maxHp; }
    void setMaxHP(int v) { _maxHp = v; }
    void heal(int n) { _hp += n; if (_hp > _maxHp) _hp = _maxHp; }

    // 等級 = XP/1000 + 1(U1 風格的經驗門檻)。升級回滿 HP + 上限成長。
    int getLevel() const { return _xp / 1000 + 1; }
    void gainXP(int n) {
        int before = getLevel();
        _xp += n;
        int after = getLevel();
        if (after > before) {
            _maxHp += 30 * (after - before);
            _hp = _maxHp;   // 升級回滿
        }
    }

    // 存檔載入用 setter(直接還原數值)
    void setHP(int v) { _hp = v; }
    void setFood(int v) { _food = v; }
    void setXP(int v) { _xp = v; }
    void setMoney(int v) { _money = v; }
    void addMoney(int v) { _money += v; }
    void addFood(int v) { _food += v; }

    // 六屬性(U1):力量/敏捷/體力/魅力/智慧/智力
    int getStrength() const { return _strength; }
    int getAgility() const { return _agility; }
    int getStamina() const { return _stamina; }
    int getCharisma() const { return _charisma; }
    int getWisdom() const { return _wisdom; }
    int getIntelligence() const { return _intelligence; }
    void setStrength(int v) { _strength = v; }
    void setAgility(int v) { _agility = v; }
    void setStamina(int v) { _stamina = v; }
    void setCharisma(int v) { _charisma = v; }
    void setWisdom(int v) { _wisdom = v; }
    void setIntelligence(int v) { _intelligence = v; }

    // 物品欄(擁有數量,index 對應 ItemCatalog):16 武器 / 8 防具 / 8 法術
    int getWeaponCount(int i) const { return (i >= 0 && i < 16) ? _weapons[i] : 0; }
    int getArmorCount(int i) const { return (i >= 0 && i < 8) ? _armor[i] : 0; }
    int getSpellCount(int i) const { return (i >= 0 && i < 8) ? _spells[i] : 0; }
    void addWeapon(int i, int n = 1) { if (i >= 0 && i < 16) _weapons[i] += n; }
    void addArmor(int i, int n = 1) { if (i >= 0 && i < 8) _armor[i] += n; }
    void addSpell(int i, int n = 1) { if (i >= 0 && i < 8) _spells[i] += n; }
    void setWeaponCount(int i, int n) { if (i >= 0 && i < 16) _weapons[i] = n; }
    void setArmorCount(int i, int n) { if (i >= 0 && i < 8) _armor[i] = n; }
    void setSpellCount(int i, int n) { if (i >= 0 && i < 8) _spells[i] = n; }

    // 目前裝備(index)
    int getCurrentWeapon() const { return _currentWeapon; }
    int getCurrentArmor() const { return _currentArmor; }
    void setCurrentWeapon(int i) { _currentWeapon = i; }
    void setCurrentArmor(int i) { _currentArmor = i; }

    // 國王任務:殺 N 隻怪。_questTarget=0 代表無任務。
    int getQuestTarget() const { return _questTarget; }
    int getQuestKills() const { return _questKills; }
    int getQuestsCompleted() const { return _questsCompleted; }
    void setQuestTarget(int v) { _questTarget = v; }
    void setQuestKills(int v) { _questKills = v; }
    void setQuestsCompleted(int v) { _questsCompleted = v; }
    bool hasQuest() const { return _questTarget > 0; }
    bool isQuestComplete() const { return _questTarget > 0 && _questKills >= _questTarget; }
    void giveQuest(int target) { _questTarget = target; _questKills = 0; }
    void recordKill() { if (_questTarget > 0 && _questKills < _questTarget) _questKills++; }
    void completeQuest() { _questTarget = 0; _questKills = 0; _questsCompleted++; }

    // 載具:筏/船(可渡水)
    bool hasRaft() const { return _hasRaft; }
    void setRaft(bool v) { _hasRaft = v; }

    // 食物消耗(隨時間 tick);耗盡回傳 true 代表飢餓
    void consumeFood(int n) { _food -= n; if (_food < 0) _food = 0; }

    bool isStarving() const { return _food <= 0; }

    bool isDead() { return _hp <= 0; }

private:
    int _overworldX;
    int _overworldY;
    int _dungeonY = 0;
    int _dungeonX = 2;
    int _dungeonLevel = 0;
    CardinalPoint _dungeonOrientation = CardinalPoint::South;
    int _townX = 0;
    int _townY = 0;

    int _hp = 150;
    int _maxHp = 150;
    int _food = 200;
    int _xp = 0;
    int _money = 100;

    // 六屬性(新角色起始值;之後由建角/升級調整)
    int _strength = 20;
    int _agility = 20;
    int _stamina = 20;
    int _charisma = 20;
    int _wisdom = 20;
    int _intelligence = 20;

    // 物品欄:預設徒手(武器 0)+ 無甲(防具 0)各 1
    int _weapons[16] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int _armor[8] = {1, 0, 0, 0, 0, 0, 0, 0};
    int _spells[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    int _currentWeapon = 0;   // 徒手
    int _currentArmor = 0;    // 無甲

    int _questTarget = 0;     // 國王任務:需殺怪數(0=無)
    int _questKills = 0;      // 已殺
    int _questsCompleted = 0; // 完成任務數(用於獎勵遞增)

    bool _hasRaft = false;    // 是否擁有筏/船(可渡水)
};

