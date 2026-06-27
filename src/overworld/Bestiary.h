#pragma once

#include "OverworldSpriteType.h"
#include <vector>

// U1 世界怪物表(考據手冊 bestiary + 官方精訊譯名)。
// 依地形分陸/海兩組;生怪時依該格地形挑一隻,給對應 sprite + 名稱 key + HP。
// HP 為 author 平衡值(弱→強),非原版逐位元對齊;名稱 key 對 assets/strings。
namespace Bestiary {

struct Mob {
    OverworldSpriteType::SpriteType sprite;
    const char *nameKey;
    int hp;
};

// 陸上怪(草原/森林):惡棍/盜賊最弱,死靈法師/黑暗騎士最強
inline const std::vector<Mob> &land() {
    using S = OverworldSpriteType::SpriteType;
    static const std::vector<Mob> v = {
        {S::HOOD, "monster.hood", 6},
        {S::THIEF, "monster.thief", 8},
        {S::ORC, "monster.orc", 12},
        {S::HIDDEN_ARCHER, "monster.hidden_archer", 12},
        {S::WANDERING_WARLOCK, "monster.wandering_warlock", 16},
        {S::EVIL_TRENT, "monster.evil_trent", 16},
        {S::BEAR, "monster.bear", 18},
        {S::EVIL_RANGER, "monster.evil_ranger", 20},
        {S::KNIGHT, "monster.knight", 22},
        {S::DARK_KNIGHT, "monster.dark_knight", 28},
        {S::NECROMANCER, "monster.necromancer", 30},
    };
    return v;
}

// 海上怪(水域):海盜船/巨烏賊/尼斯水怪/火龍龜,均強悍
inline const std::vector<Mob> &sea() {
    using S = OverworldSpriteType::SpriteType;
    static const std::vector<Mob> v = {
        {S::PIRATE_SHIP, "monster.pirate_ship", 20},
        {S::GIANT_SQUID, "monster.giant_squid", 30},
        {S::NESS_MONSTER, "monster.ness", 35},
        {S::DRAGON_TURTLE, "monster.dragon_turtle", 45},
    };
    return v;
}

// 依地形挑一隻;r 為隨機數(呼叫端給 rand())
inline const Mob &pick(bool isWater, unsigned r) {
    const auto &v = isWater ? sea() : land();
    return v[r % v.size()];
}

} // namespace Bestiary
