#pragma once

#include <cstdlib>
#include "Player.h"
#include "ItemCatalog.h"

// 戰鬥傷害公式(Phase B):接上裝備/屬性。數值先求可玩合理,逐步對齊原版。
namespace Combat {
    // 玩家攻擊傷害 = 基礎 + 武器威力 + 力量加成 + 隨機浮動
    inline int playerAttackDamage(const Player &p) {
        return 2 + ItemCatalog::weaponPower(p.getCurrentWeapon()) + p.getStrength() / 5 + (rand() % 4);
    }

    // 受到的傷害經防具減免(至少 1)
    inline int reduceByArmor(int raw, const Player &p) {
        int d = raw - ItemCatalog::armorDefense(p.getCurrentArmor());
        return d < 1 ? 1 : d;
    }

    // 擊殺怪物獲得的經驗(以怪物名長度/型別粗估;後續可對照 bestiary 表)
    inline int killXP(int enemyMaxHp) {
        return 50 + enemyMaxHp * 5;
    }
}
