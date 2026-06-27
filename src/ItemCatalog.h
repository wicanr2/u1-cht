#pragma once

#include <vector>
#include <string>

// 商店品項目錄:武器/防具/食物/魔法的可購買清單(名稱走 i18n key,index 對應 Player 物品欄)。
// 數值先 author 自手冊/wiki(可玩、合理),逐步對齊原版(見 docs/plan-town-combat.md)。
enum class ShopType { Weapon, Armor, Food, Magic, Pub, Transport, None };

struct ShopItem {
    int index;            // Player 物品欄 index(武器/防具/法術)
    std::string nameKey;  // i18n key
    int price;            // 金幣
};

class ItemCatalog {
public:
    // 武器名(i18n key),index 0..15
    static std::string weaponNameKey(int i) {
        static const char *k[16] = {
            "item.weapon.hands", "item.weapon.dagger", "item.weapon.mace", "item.weapon.axe",
            "item.weapon.bow", "item.weapon.sword", "item.weapon.greatsword", "item.weapon.phazor",
            "item.weapon.w8", "item.weapon.w9", "item.weapon.w10", "item.weapon.w11",
            "item.weapon.w12", "item.weapon.w13", "item.weapon.w14", "item.weapon.w15"};
        return (i >= 0 && i < 16) ? k[i] : "item.weapon.hands";
    }
    static std::string armorNameKey(int i) {
        static const char *k[8] = {
            "item.armor.none", "item.armor.leather", "item.armor.chain", "item.armor.plate",
            "item.armor.a4", "item.armor.a5", "item.armor.a6", "item.armor.a7"};
        return (i >= 0 && i < 8) ? k[i] : "item.armor.none";
    }
    static std::string spellNameKey(int i) {
        static const char *k[8] = {
            "item.spell.prayer", "item.spell.open", "item.spell.unlock", "item.spell.magic_missile",
            "item.spell.ladder_up", "item.spell.ladder_down", "item.spell.blink", "item.spell.kill"};
        return (i >= 0 && i < 8) ? k[i] : "item.spell.prayer";
    }

    // 攻擊/防禦力(index 越大越強)— 戰鬥公式 Phase B 用
    static int weaponPower(int i) { return i * 4; }      // 徒手 0、匕首 4、…
    static int armorDefense(int i) { return i * 3; }     // 無甲 0、皮甲 3、…

    // 各店品項(index, nameKey, price)
    static std::vector<ShopItem> itemsFor(ShopType t) {
        switch (t) {
            case ShopType::Weapon:
                return {{1, weaponNameKey(1), 30}, {2, weaponNameKey(2), 75}, {3, weaponNameKey(3), 125},
                        {4, weaponNameKey(4), 175}, {5, weaponNameKey(5), 250}, {6, weaponNameKey(6), 400},
                        {7, weaponNameKey(7), 800}};
            case ShopType::Armor:
                return {{1, armorNameKey(1), 40}, {2, armorNameKey(2), 120}, {3, armorNameKey(3), 250},
                        {4, armorNameKey(4), 500}};
            case ShopType::Food:
                // 食物:固定一筆,每買一次 +50 食物
                return {{0, "item.food.rations", 20}};
            case ShopType::Magic:
                return {{3, spellNameKey(3), 100}, {1, spellNameKey(1), 60}, {2, spellNameKey(2), 60},
                        {6, spellNameKey(6), 150}, {7, spellNameKey(7), 300}};
            case ShopType::Pub:
                return {{0, "shop.pub.drink", 10}};   // 買一杯,聽段八卦/線索
            case ShopType::Transport:
                // index 對應 Player::Vehicle(1=馬 2=木筏 3=巡防艦 4=飛車)
                return {{1, "item.transport.horse", 100},
                        {2, "item.transport.raft", 200},
                        {3, "item.transport.frigate", 500},
                        {4, "item.transport.aircar", 1500},
                        {9, "item.transport.shuttle", 1000}};   // 9=太空梭(非地面載具,L 鍵發射)
            default:
                return {};
        }
    }

    static std::string shopTitleKey(ShopType t) {
        switch (t) {
            case ShopType::Weapon: return "shop.title.weapon";
            case ShopType::Armor:  return "shop.title.armor";
            case ShopType::Food:   return "shop.title.food";
            case ShopType::Magic:  return "shop.title.magic";
            case ShopType::Pub:    return "shop.title.pub";
            case ShopType::Transport: return "shop.title.transport";
            default: return "shop.title.none";
        }
    }
};
