#pragma once

#include <array>
#include <string>
#include "Player.h"

// U1 建角開場資料:性別 / 種族 / 職業 + 30 點屬性分配。
// 數值為 author 自手冊精神(平衡可玩),非逐位元對齊原版;種族給屬性修正,職業給主屬性傾向。
// 六屬性順序固定:力量/敏捷/體力/魅力/智慧/智力(對應 Player 的 6 個 setter)。
namespace CharCreation {

constexpr int kAttrN = 6;          // 力量/敏捷/體力/魅力/智慧/智力
constexpr int kBase = 10;          // 每屬性基礎值
constexpr int kPool = 30;          // 可分配點數
constexpr int kAttrCap = 25;       // 單屬性上限

inline const char *attrKey(int i) {
    static const char *k[kAttrN] = {
        "cc.attr.str", "cc.attr.agi", "cc.attr.sta",
        "cc.attr.cha", "cc.attr.wis", "cc.attr.int"};
    return k[(i >= 0 && i < kAttrN) ? i : 0];
}

// 性別
constexpr int kSexN = 2;
inline const char *sexKey(int i) {
    static const char *k[kSexN] = {"cc.sex.male", "cc.sex.female"};
    return k[(i >= 0 && i < kSexN) ? i : 0];
}

// 種族 + 屬性修正(力,敏,體,魅,智,智力)
struct Race { const char *nameKey; std::array<int, kAttrN> mod; };
constexpr int kRaceN = 5;
inline const Race &race(int i) {
    static const Race r[kRaceN] = {
        {"cc.race.human", {0, 0, 0, 0, 0, 0}},     // 人類:均衡
        {"cc.race.elf",   {-2, 2, -1, 0, 2, 3}},   // 精靈:智/敏高、力低
        {"cc.race.dwarf", {3, -1, 3, -1, 0, -2}},  // 矮人:力/體高、智低
        {"cc.race.bobbit",{-1, 3, 0, 2, 0, 0}},    // 哈比:敏/魅高
        {"cc.race.fuzzy", {-1, 0, -1, 3, 3, 1}},   // 毛茸族:魅/智高、力低
    };
    return r[(i >= 0 && i < kRaceN) ? i : 0];
}

// 職業 + 起始裝備/法術傾向(主屬性僅作 UI 提示;裝備在 apply 給)
struct Klass { const char *nameKey; int primeAttr; int startWeapon; int startArmor; int startSpell; };
constexpr int kKlassN = 4;
inline const Klass &klass(int i) {
    static const Klass k[kKlassN] = {
        {"cc.class.fighter", 0, 5, 2, -1},  // 戰士:主力量,起始 劍+鎖甲
        {"cc.class.cleric",  4, 2, 1, 0},   // 牧師:主智慧,起始 釘錘+皮甲+祈禱
        {"cc.class.wizard",  5, 1, 0, 3},   // 巫師:主智力,起始 匕首+無甲+飛彈
        {"cc.class.thief",   1, 1, 1, -1},  // 盜賊:主敏捷,起始 匕首+皮甲
    };
    return k[(i >= 0 && i < kKlassN) ? i : 0];
}

// 把建角選擇套到 Player:基礎+種族修正+分配點 → 六屬性;職業 → 起始裝備/法術。
inline void apply(Player &p, int sex, int raceIdx, int klassIdx,
                  const std::array<int, kAttrN> &alloc) {
    (void)sex;
    const Race &r = race(raceIdx);
    int a[kAttrN];
    for (int i = 0; i < kAttrN; ++i) {
        a[i] = kBase + r.mod[i] + alloc[i];
        if (a[i] < 1) a[i] = 1;
        if (a[i] > kAttrCap) a[i] = kAttrCap;
    }
    p.setStrength(a[0]); p.setAgility(a[1]); p.setStamina(a[2]);
    p.setCharisma(a[3]); p.setWisdom(a[4]); p.setIntelligence(a[5]);

    const Klass &k = klass(klassIdx);
    if (k.startWeapon >= 0) { p.addWeapon(k.startWeapon); p.setCurrentWeapon(k.startWeapon); }
    if (k.startArmor > 0)   { p.addArmor(k.startArmor); p.setCurrentArmor(k.startArmor); }
    if (k.startSpell >= 0)  p.addSpell(k.startSpell);
    // 體力略增初始 HP 上限(體格反映耐久)
    int hp = 100 + a[2] * 2;
    p.setMaxHP(hp); p.setHP(hp);
}

} // namespace CharCreation
