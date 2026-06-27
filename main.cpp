#include <SDL.h>
#include <SDL_image.h>
#include <cstdio>
#include "src/overworld/OverworldTile.h"
#include "src/common/graphics/CGALinearDecodeStrategy.h"
#include "src/common/graphics/EGARowPlanarDecodeStrategy.h"
#include <memory>
#include "src/common/LTimer.h"
#include "src/overworld/OverworldScreen.h"
#include "src/common/Fonts.h"
#include "src/common/Audio.h"
#include "src/common/graphics/LTexture.h"
#include "src/Constants.h"
#include "src/CommandDisplay.h"
#include "src/dungeon/DungeonScreen.h"
#include "src/town/TownManager.h"
#include "src/town/TownScreen.h"
#include "src/town/TownSpriteTypeLoader.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include "src/Configuration.h"
#include "src/SaveGame.h"
#include "src/common/I18n.h"
#include "src/ItemCatalog.h"
#include "src/CharCreation.h"
#include <array>

using namespace std;
using namespace OpenUltima;

//Starts up SDL and creates window
bool init();

//Loads media
bool loadMedia();

//Frees media and shuts down SDL
void close();

//The window we'll be rendering to
SDL_Window *gWindow = nullptr;

//The window renderer
SDL_Renderer *gRenderer = nullptr;

// 拉高內部畫布:世界(tile/sprite)先畫進原生 320x200 離屏 target,
// 整張用 nearest 放大到 640x400,讓中文 UI 有 2x 空間清晰繪製(retro-cjk-hires-canvas)。
constexpr int GAME_W = 320, GAME_H = 200;
constexpr int CANVAS_SCALE = 2;
constexpr int CANVAS_W = GAME_W * CANVAS_SCALE, CANVAS_H = GAME_H * CANVAS_SCALE;
SDL_Texture *gWorldTarget = nullptr;

// 離開確認對話框:置中 modal + 半透明 scrim(640x400 覆蓋層空間繪製)
static void drawQuitDialog(SDL_Renderer *renderer) {
    // 暗化背景
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xB0);
    SDL_Rect scrim = {0, 0, CANVAS_W, CANVAS_H};
    SDL_RenderFillRect(renderer, &scrim);

    // 對話框
    const int bw = 360, bh = 130;
    SDL_Rect box = {(CANVAS_W - bw) / 2, (CANVAS_H - bh) / 2, bw, bh};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
    SDL_RenderFillRect(renderer, &box);
    SDL_SetRenderDrawColor(renderer, 0x40, 0x80, 0xFF, 0xFF);  // 藍邊
    SDL_RenderDrawRect(renderer, &box);

    auto line = [&](const string &s, int y, SDL_Color c) {
        LTexture t;
        t.loadFromRenderedText(Fonts::cjkUi(), renderer, s, c);
        t.render(renderer, box.x + (bw - t.getWidth()) / 2, y);
    };
    line(I18n::t("ui.quit.confirm"), box.y + 22, SDL_Color{0xFF, 0xD0, 0x40, 0xFF});   // accent 標題
    line(I18n::t("ui.quit.yes"), box.y + 60, SDL_Color{0xFF, 0xFF, 0xFF, 0xFF});
    line(I18n::t("ui.quit.no"), box.y + 88, SDL_Color{0xC0, 0xC0, 0xC0, 0xFF});
}

// F6 設定選單:即時調 speed_pct / spawn_pct(置中 modal,640x400 覆蓋層)
static void drawSettingsMenu(SDL_Renderer *renderer, int row) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xB0);
    SDL_Rect scrim = {0, 0, CANVAS_W, CANVAS_H};
    SDL_RenderFillRect(renderer, &scrim);

    const int bw = 440, bh = 210;
    SDL_Rect box = {(CANVAS_W - bw) / 2, (CANVAS_H - bh) / 2, bw, bh};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
    SDL_RenderFillRect(renderer, &box);
    SDL_SetRenderDrawColor(renderer, 0x40, 0x80, 0xFF, 0xFF);
    SDL_RenderDrawRect(renderer, &box);

    auto line = [&](const string &s, int x, int y, SDL_Color c) {
        LTexture t; t.loadFromRenderedText(Fonts::cjkUi(), renderer, s, c);
        t.render(renderer, x, y);
    };
    SDL_Color title{0xFF, 0xD0, 0x40, 0xFF}, on{0xFF, 0xFF, 0x60, 0xFF}, off{0xC0, 0xC0, 0xC0, 0xFF};
    line(I18n::t("ui.settings.title"), box.x + (bw - 120) / 2, box.y + 16, title);
    line((row == 0 ? "▶ " : "  ") + (I18n::t("ui.settings.speed") + to_string(Configuration::getSpeedPct()) + "%"),
         box.x + 40, box.y + 56, row == 0 ? on : off);
    line((row == 1 ? "▶ " : "  ") + (I18n::t("ui.settings.spawn") + to_string(Configuration::getSpawnPct()) + "%"),
         box.x + 40, box.y + 88, row == 1 ? on : off);
    line((row == 2 ? "▶ " : "  ") + (I18n::t("ui.settings.chase") + string(Configuration::getChaseMonsters() ? I18n::t("ui.on") : I18n::t("ui.off"))),
         box.x + 40, box.y + 120, row == 2 ? on : off);
    line(I18n::t("ui.settings.hint"), box.x + 36, box.y + 168, off);
}

// F1 說明畫面:列出全部指令(置中 modal,640x400 覆蓋層)
static void drawHelpScreen(SDL_Renderer *renderer) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xC0);
    SDL_Rect scrim = {0, 0, CANVAS_W, CANVAS_H};
    SDL_RenderFillRect(renderer, &scrim);

    const int bw = 540, bh = 522;
    SDL_Rect box = {(CANVAS_W - bw) / 2, (CANVAS_H - bh) / 2, bw, bh};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
    SDL_RenderFillRect(renderer, &box);
    SDL_SetRenderDrawColor(renderer, 0x40, 0x80, 0xFF, 0xFF);
    SDL_RenderDrawRect(renderer, &box);

    SDL_Color title{0xFF, 0xD0, 0x40, 0xFF}, head{0x80, 0xC0, 0xFF, 0xFF},
        key{0xFF, 0xFF, 0x80, 0xFF}, txt{0xE0, 0xE0, 0xE0, 0xFF};
    auto line = [&](const string &s, int x, int y, SDL_Color c) {
        LTexture t; t.loadFromRenderedText(Fonts::cjkUi(), renderer, s, c);
        t.render(renderer, x, y);
    };
    int lx = box.x + 30, kx = box.x + 50, tx = box.x + 200, y = box.y + 18;
    line(I18n::t("ui.help.title"), box.x + (bw - 140) / 2, y, title); y += 38;
    line(I18n::t("ui.help.move_head"), lx, y, head); y += 28;
    line(I18n::t("ui.help.arrows"), kx, y, key); line(I18n::t("ui.help.arrows_desc"), tx, y, txt); y += 30;
    line(I18n::t("ui.help.action_head"), lx, y, head); y += 28;
    line("A", kx, y, key); line(I18n::t("ui.help.attack"), tx, y, txt); y += 26;
    line("E", kx, y, key); line(I18n::t("ui.help.enter"), tx, y, txt); y += 26;
    line("K", kx, y, key); line(I18n::t("ui.help.klimb"), tx, y, txt); y += 26;
    line("B", kx, y, key); line(I18n::t("ui.help.buy"), tx, y, txt); y += 26;
    line("T", kx, y, key); line(I18n::t("ui.help.talk"), tx, y, txt); y += 26;
    line("C", kx, y, key); line(I18n::t("ui.help.cast"), tx, y, txt); y += 26;
    line("Z", kx, y, key); line(I18n::t("ui.help.ztats"), tx, y, txt); y += 30;
    line(I18n::t("ui.help.sys_head"), lx, y, head); y += 28;
    line("F6", kx, y, key); line(I18n::t("ui.help.f6"), tx, y, txt); y += 24;
    line("M  /  F9", kx, y, key); line(I18n::t("ui.help.audio"), tx, y, txt); y += 24;
    line("PageDown", kx, y, key); line(I18n::t("ui.help.tileset"), tx, y, txt); y += 24;
    line("F5", kx, y, key); line(I18n::t("ui.help.save"), tx, y, txt); y += 24;
    line("F10 / Ctrl+Q", kx, y, key); line(I18n::t("ui.help.quit"), tx, y, txt); y += 24;
    line("ESC", kx, y, key); line(I18n::t("ui.help.esc"), tx, y, txt); y += 30;
    line(I18n::t("ui.help.dismiss"), box.x + (bw - 120) / 2, box.y + bh - 30, SDL_Color{0xA0, 0xA0, 0xA0, 0xFF});
}

// 商店:mode 1=類別選單、mode 2=品項清單(置中 modal)
static const int kShopCatN = 6;
static const ShopType kShopCats[kShopCatN] = {ShopType::Weapon, ShopType::Armor, ShopType::Food,
                                              ShopType::Magic, ShopType::Pub, ShopType::Transport};
static void drawShop(SDL_Renderer *renderer, Player &player, int mode, ShopType type, int sel) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xC0);
    SDL_Rect scrim = {0, 0, CANVAS_W, CANVAS_H};
    SDL_RenderFillRect(renderer, &scrim);

    const int bw = 460, bh = 320;
    SDL_Rect box = {(CANVAS_W - bw) / 2, (CANVAS_H - bh) / 2, bw, bh};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
    SDL_RenderFillRect(renderer, &box);
    SDL_SetRenderDrawColor(renderer, 0x40, 0x80, 0xFF, 0xFF);
    SDL_RenderDrawRect(renderer, &box);

    SDL_Color title{0xFF, 0xD0, 0x40, 0xFF}, on{0xFF, 0xFF, 0x60, 0xFF},
        off{0xC0, 0xC0, 0xC0, 0xFF}, gold{0xFF, 0xE0, 0x80, 0xFF};
    auto line = [&](const string &s, int x, int y, SDL_Color c) {
        LTexture t; t.loadFromRenderedText(Fonts::cjkUi(), renderer, s, c);
        t.render(renderer, x, y);
    };
    int y = box.y + 18;
    line(I18n::t("shop.greeting"), box.x + 24, y, title);
    line(I18n::tf("shop.gold", {to_string(player.getMoney())}), box.x + bw - 150, y, gold);
    y += 40;
    if (mode == 1) {
        for (int i = 0; i < kShopCatN; ++i) {
            line((sel == i ? "▶ " : "  ") + I18n::t(ItemCatalog::shopTitleKey(kShopCats[i])),
                 box.x + 40, y, sel == i ? on : off);
            y += 34;
        }
        line(I18n::t("shop.hint_cat"), box.x + 30, box.y + bh - 30, off);
    } else {
        line(I18n::t(ItemCatalog::shopTitleKey(type)), box.x + 24, y, title); y += 32;
        auto items = ItemCatalog::itemsFor(type);
        for (int i = 0; i < (int)items.size(); ++i) {
            line((sel == i ? "▶ " : "  ") + I18n::t(items[i].nameKey) + "  " +
                     to_string(items[i].price) + " G",
                 box.x + 40, y, sel == i ? on : off);
            y += 28;
        }
        line(I18n::t("shop.hint_buy"), box.x + 30, box.y + bh - 30, off);
    }
}

// 國王任務數值(draw/input 共用):討伐數 / 金幣賞
static int questTargetFor(Player &p) { return 5 + p.getQuestsCompleted() * 5; }
static int questGoldFor(Player &p) { return 100 + p.getQuestsCompleted() * 100; }
static const int kEndgameQuests = 4;   // 完成 4 位國王試煉 → 終局
static bool endgameReady(Player &p) { return p.getQuestsCompleted() >= kEndgameQuests && !p.hasQuest(); }

// 城堡國王對話(置中 modal)
static void drawKing(SDL_Renderer *renderer, Player &player) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xC0);
    SDL_Rect scrim = {0, 0, CANVAS_W, CANVAS_H};
    SDL_RenderFillRect(renderer, &scrim);
    const int bw = 520, bh = 220;
    SDL_Rect box = {(CANVAS_W - bw) / 2, (CANVAS_H - bh) / 2, bw, bh};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
    SDL_RenderFillRect(renderer, &box);
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xD0, 0x40, 0xFF);   // 金邊(王者)
    SDL_RenderDrawRect(renderer, &box);
    SDL_Color title{0xFF, 0xD0, 0x40, 0xFF}, txt{0xE0, 0xE0, 0xE0, 0xFF}, hint{0xA0, 0xA0, 0xA0, 0xFF};
    auto line = [&](const string &s, int x, int y, SDL_Color c) {
        LTexture t; t.loadFromRenderedText(Fonts::cjkUi(), renderer, s, c); t.render(renderer, x, y);
    };
    line(I18n::t("king.title"), box.x + (bw - 120) / 2, box.y + 18, title);
    if (endgameReady(player))
        line(I18n::t("king.endgame"), box.x + 30, box.y + 70, SDL_Color{0xFF, 0xA0, 0x40, 0xFF});
    else if (!player.hasQuest())
        line(I18n::tf("king.offer", {to_string(questTargetFor(player)), to_string(questGoldFor(player))}),
             box.x + 30, box.y + 70, txt);
    else if (player.isQuestComplete())
        line(I18n::t("king.complete"), box.x + 30, box.y + 70, txt);
    else
        line(I18n::tf("king.incomplete", {to_string(player.getQuestKills()), to_string(player.getQuestTarget())}),
             box.x + 30, box.y + 70, txt);
    bool actionable = !player.hasQuest() || player.isQuestComplete() || endgameReady(player);
    line(I18n::t(actionable ? "king.hint_action" : "king.hint_close"), box.x + 30, box.y + bh - 32, hint);
}

// 勝利畫面:擊敗蒙登、索薩利亞重歸光明(置中 modal,任意鍵關閉)
static void drawVictory(SDL_Renderer *renderer) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xF0);
    SDL_Rect scrim = {0, 0, CANVAS_W, CANVAS_H};
    SDL_RenderFillRect(renderer, &scrim);
    const int bw = 600, bh = 320;
    SDL_Rect box = {(CANVAS_W - bw) / 2, (CANVAS_H - bh) / 2, bw, bh};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
    SDL_RenderFillRect(renderer, &box);
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xD0, 0x40, 0xFF);
    SDL_RenderDrawRect(renderer, &box);
    SDL_Color gold{0xFF, 0xD0, 0x40, 0xFF}, txt{0xF0, 0xF0, 0xF0, 0xFF};
    auto center = [&](const string &s, int yy, SDL_Color c) {
        LTexture t; t.loadFromRenderedText(Fonts::cjkUi(), renderer, s, c);
        t.render(renderer, box.x + (bw - t.getWidth()) / 2, yy);
    };
    int y = box.y + 30;
    center(I18n::t("victory.title"), y, gold); y += 56;
    for (int i = 0; i < 4; ++i) { center(I18n::t("victory.line" + to_string(i)), y, txt); y += 36; }
    center(I18n::t("ui.help.dismiss"), box.y + bh - 34, SDL_Color{0xA0, 0xA0, 0xA0, 0xFF});
}

// 取玩家擁有的法術 index 清單
static std::vector<int> ownedSpells(Player &p) {
    std::vector<int> v;
    for (int i = 0; i < 8; ++i) if (p.getSpellCount(i) > 0) v.push_back(i);
    return v;
}

// 施法選單(置中 modal)
static void drawCast(SDL_Renderer *renderer, Player &player, int sel) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xC0);
    SDL_Rect scrim = {0, 0, CANVAS_W, CANVAS_H};
    SDL_RenderFillRect(renderer, &scrim);
    const int bw = 420, bh = 280;
    SDL_Rect box = {(CANVAS_W - bw) / 2, (CANVAS_H - bh) / 2, bw, bh};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
    SDL_RenderFillRect(renderer, &box);
    SDL_SetRenderDrawColor(renderer, 0xC0, 0x80, 0xFF, 0xFF);   // 紫邊(魔法)
    SDL_RenderDrawRect(renderer, &box);
    SDL_Color title{0xC0, 0x90, 0xFF, 0xFF}, on{0xFF, 0xFF, 0x60, 0xFF}, off{0xC0, 0xC0, 0xC0, 0xFF};
    auto line = [&](const string &s, int x, int y, SDL_Color c) {
        LTexture t; t.loadFromRenderedText(Fonts::cjkUi(), renderer, s, c); t.render(renderer, x, y);
    };
    line(I18n::t("spell.menu_title"), box.x + (bw - 100) / 2, box.y + 18, title);
    auto owned = ownedSpells(player);
    int y = box.y + 60;
    if (owned.empty()) {
        line(I18n::t("spell.empty"), box.x + 40, y, off);
    } else {
        for (int i = 0; i < (int)owned.size(); ++i) {
            line((sel == i ? "▶ " : "  ") + I18n::t(ItemCatalog::spellNameKey(owned[i])) +
                     "  ×" + to_string(player.getSpellCount(owned[i])),
                 box.x + 40, y, sel == i ? on : off);
            y += 28;
        }
    }
    line(I18n::t("spell.hint"), box.x + 30, box.y + bh - 30, off);
}

// Z 角色屬性表(Ztats):等級/屬性/裝備/任務(置中 modal,任意鍵關閉)
static void drawZtats(SDL_Renderer *renderer, Player &p) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xC8);
    SDL_Rect scrim = {0, 0, CANVAS_W, CANVAS_H};
    SDL_RenderFillRect(renderer, &scrim);
    const int bw = 540, bh = 400;
    SDL_Rect box = {(CANVAS_W - bw) / 2, (CANVAS_H - bh) / 2, bw, bh};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
    SDL_RenderFillRect(renderer, &box);
    SDL_SetRenderDrawColor(renderer, 0x40, 0x80, 0xFF, 0xFF);
    SDL_RenderDrawRect(renderer, &box);
    SDL_Color title{0xFF, 0xD0, 0x40, 0xFF}, head{0x80, 0xC0, 0xFF, 0xFF},
        lab{0xC0, 0xC0, 0xC0, 0xFF}, val{0xFF, 0xFF, 0xFF, 0xFF};
    auto line = [&](const string &s, int x, int yy, SDL_Color c) {
        LTexture t; t.loadFromRenderedText(Fonts::cjkUi(), renderer, s, c); t.render(renderer, x, yy);
    };
    int c1 = box.x + 40, c2 = box.x + 290, y = box.y + 18;
    line(I18n::t("ztats.title"), box.x + (bw - 120) / 2, y, title); y += 40;
    auto kv = [&](const string &k, const string &v, int col, int yy) {
        line(I18n::t(k), col, yy, lab); line(v, col + 110, yy, val);
    };
    kv("ztats.level", to_string(p.getLevel()), c1, y);
    kv("ztats.hp", to_string(p.getHP()) + "/" + to_string(p.getMaxHP()), c2, y); y += 28;
    kv("ztats.xp", to_string(p.getXP()), c1, y);
    kv("ztats.gold", to_string(p.getMoney()), c2, y); y += 36;
    line(I18n::t("ztats.attrs"), c1, y, head); y += 28;
    kv("ztats.str", to_string(p.getStrength()), c1, y);
    kv("ztats.agi", to_string(p.getAgility()), c2, y); y += 26;
    kv("ztats.sta", to_string(p.getStamina()), c1, y);
    kv("ztats.cha", to_string(p.getCharisma()), c2, y); y += 26;
    kv("ztats.wis", to_string(p.getWisdom()), c1, y);
    kv("ztats.int", to_string(p.getIntelligence()), c2, y); y += 36;
    line(I18n::t("ztats.equip"), c1, y, head); y += 28;
    kv("ztats.weapon", I18n::t(ItemCatalog::weaponNameKey(p.getCurrentWeapon())), c1, y);
    kv("ztats.armor", I18n::t(ItemCatalog::armorNameKey(p.getCurrentArmor())), c2, y); y += 32;
    if (p.hasQuest())
        line(I18n::tf("ztats.quest", {to_string(p.getQuestKills()), to_string(p.getQuestTarget())}), c1, y, lab);
    else
        line(I18n::t("ztats.no_quest"), c1, y, lab);
    line(I18n::t("ui.help.dismiss"), box.x + (bw - 120) / 2, box.y + bh - 28, SDL_Color{0xA0, 0xA0, 0xA0, 0xFF});
}

// 建角狀態:性別 / 種族 / 職業 + 6 屬性分配 + 目前選列(0=性別,1=種族,2=職業,3..8=屬性,9=開始)
struct CharCreateState {
    int sex = 0, raceIdx = 0, klassIdx = 0, row = 0;
    std::array<int, CharCreation::kAttrN> alloc{0, 0, 0, 0, 0, 0};
    int spent() const { int s = 0; for (int v : alloc) s += v; return s; }
    int remaining() const { return CharCreation::kPool - spent(); }
};
constexpr int kCCRowStart = 3 + CharCreation::kAttrN;  // 開始列 index = 9
constexpr int kCCRowN = kCCRowStart + 1;               // 10 列

// 建角畫面:置中 modal,顯示性別/種族/職業選擇 + 六屬性分配 + 剩餘點數。
static void drawCharCreate(SDL_Renderer *renderer, const CharCreateState &s) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
    SDL_Rect full = {0, 0, CANVAS_W, CANVAS_H};
    SDL_RenderFillRect(renderer, &full);
    const int bw = 560, bh = 420;
    SDL_Rect box = {(CANVAS_W - bw) / 2, (CANVAS_H - bh) / 2, bw, bh};
    SDL_SetRenderDrawColor(renderer, 0x10, 0x10, 0x18, 0xFF);
    SDL_RenderFillRect(renderer, &box);
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xD0, 0x40, 0xFF);
    SDL_RenderDrawRect(renderer, &box);
    SDL_Color title{0xFF, 0xD0, 0x40, 0xFF}, lab{0xC0, 0xC0, 0xC0, 0xFF},
        val{0xFF, 0xFF, 0xFF, 0xFF}, sel{0xFF, 0xFF, 0x60, 0xFF}, dim{0x80, 0x80, 0x80, 0xFF};
    auto line = [&](const string &str, int x, int yy, SDL_Color c) {
        LTexture t; t.loadFromRenderedText(Fonts::cjkUi(), renderer, str, c); t.render(renderer, x, yy);
    };
    int x = box.x + 40, y = box.y + 18;
    line(I18n::t("cc.title"), box.x + (bw - 80) / 2, y, title); y += 42;

    // 一列:label + ◄ value ►(選中時黃字 + 箭頭)
    auto picker = [&](int rowIdx, const string &labelKey, const string &value) {
        bool on = (s.row == rowIdx);
        line(I18n::t(labelKey), x, y, on ? sel : lab);
        string v = on ? ("◀ " + value + " ▶") : ("  " + value);
        line(v, x + 130, y, on ? sel : val);
        y += 30;
    };
    picker(0, "cc.row.sex", I18n::t(CharCreation::sexKey(s.sex)));
    picker(1, "cc.row.race", I18n::t(CharCreation::race(s.raceIdx).nameKey));
    {
        const auto &k = CharCreation::klass(s.klassIdx);
        string cls = I18n::t(k.nameKey) + "  " + I18n::t("cc.prime") + I18n::t(CharCreation::attrKey(k.primeAttr));
        picker(2, "cc.row.class", cls);
    }
    y += 8;
    line(I18n::tf("cc.points", {to_string(s.remaining())}), x, y,
         s.remaining() > 0 ? SDL_Color{0x80, 0xFF, 0x80, 0xFF} : dim); y += 30;

    // 六屬性:有效值 = base + 種族修正 + 分配
    const auto &r = CharCreation::race(s.raceIdx);
    for (int i = 0; i < CharCreation::kAttrN; ++i) {
        bool on = (s.row == 3 + i);
        int eff = CharCreation::kBase + r.mod[i] + s.alloc[i];
        line(I18n::t(CharCreation::attrKey(i)), x, y, on ? sel : lab);
        string bar = (on ? "◀ " : "  ") + to_string(eff) + (on ? " ▶" : "");
        line(bar, x + 130, y, on ? sel : val);
        if (s.alloc[i] > 0) line("+" + to_string(s.alloc[i]), x + 230, y, dim);
        y += 26;
    }
    y += 12;
    bool startOn = (s.row == kCCRowStart);
    line(I18n::t("cc.start"), box.x + (bw - 130) / 2, y, startOn ? sel : dim); y += 30;
    line(I18n::t("cc.hint"), box.x + 40, box.y + bh - 30, dim);
}

shared_ptr<PlayerStatusDisplay> _playerStatusDisplay;
shared_ptr<CommandDisplay> _commandDisplay;

bool init() {
    //Initialization flag
    bool success = true;

    //Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        success = false;
    } else {
        //Set texture filtering to linear
        if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 0)) {
            printf("Warning: Linear texture filtering not enabled!");
        }

        //Create window
        gWindow = SDL_CreateWindow(I18n::t("window.title").c_str(), SDL_WINDOWPOS_UNDEFINED,
                                   SDL_WINDOWPOS_UNDEFINED, Configuration::getScreenWidth(),
                                   Configuration::getScreenHeight(), SDL_WINDOW_SHOWN);
        if (gWindow == NULL) {
            printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
            success = false;
        } else {
            //Create renderer for window
            gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            if (gRenderer == NULL) {
                printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
                success = false;
            } else {
                //Initialize renderer color
                SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

                //Initialize PNG loading
                int imgFlags = IMG_INIT_PNG;
                if (!(IMG_Init(imgFlags) & imgFlags)) {
                    printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
                    success = false;
                }
            }
        }

        //Initialize SDL_ttf
        if (TTF_Init() == -1) {
            printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
            success = false;
        }
    }

    return success;
}

bool loadMedia() {
    //Loading success flag
    bool success = true;

    //Nothing to load
    return success;
}

void close() {
    _commandDisplay = nullptr;
    _playerStatusDisplay = nullptr;

    //Destroy window
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    gWindow = NULL;
    gRenderer = NULL;

    //Quit SDL subsystems
    Audio::shutdown();
    IMG_Quit();
    SDL_Quit();
}

int main(int argc, char *args[]) {
    Configuration::init();
    I18n::init("assets/strings/" + Configuration::getLanguage() + ".json");
    cout << Configuration::getEgaOverworldTilesFilePath();

    //Start up SDL and create window
    if (!init()) {
        printf("Failed to initialize!\n");
    } else {
        //Load media
        if (!loadMedia()) {
            printf("Failed to load media!\n");
        } else {
            //Main loop flag
            bool quit = false;

            //Event handler
            SDL_Event e;

            auto player = make_shared<Player>(20, 20);
            // 啟動時若有存檔則載入(還原玩家位置/數值 + F6 設定);損毀則維持新遊戲。
            // 無存檔 = 新遊戲 → 之後跑建角開場(needCharCreate)。測試 hook 與 U1_SKIP_CC 一律跳過。
            bool hasSave = SaveGame::exists(SaveGame::defaultPath());
            if (hasSave) SaveGame::load(*player, SaveGame::defaultPath());
            bool needCharCreate = !hasSave && !getenv("U1_SKIP_CC") &&
                                  !getenv("U1_TEST_DUNGEON") && !getenv("U1_TEST_TOWN") &&
                                  !getenv("U1_TEST_CASTLE");
            auto gameContext = make_shared<GameContext>(player);
            // 測試 hook(env-gated,正常遊玩不啟用):直接進地牢驗證怪物移動
            if (getenv("U1_TEST_DUNGEON")) gameContext->setScreen(ScreenType::Dungeon);
            if (getenv("U1_TEST_TOWN")) {
                player->setOverworldX(65); player->setOverworldY(22);  // 真實城鎮座標
                gameContext->setScreen(ScreenType::Town);
            }
            if (getenv("U1_TEST_CASTLE")) {
                player->setOverworldX(32); player->setOverworldY(27);  // 城堡座標
                gameContext->enterCastle();
            }
            auto overworldScreen = make_shared<OverworldScreen>(gameContext, 19, 9);
            auto dungeonScreen = make_shared<DungeonScreen>(gameContext);

            auto egaTilesPath = Configuration::getEgaOverworldTilesFilePath();
            auto cgaTilesPath = Configuration::getCgaOverworldTilesFilePath();

            // tileset 變體:0=EGA(BIN),1=CGA(BIN),2..=各跨平台 PNG 素材包(F1 逐一循環)。
            // music = 該平台原版 BGM(版權,使用者自備於 assets/music/,缺檔則 fallback 占位曲)。
            struct Pack { std::string name, tiles, music; };
            const std::string kDefaultMusic = "./assets/music/theme.ogg";
            std::vector<Pack> pngPacks = {
                {"FM Towns", "assets/tilesets/fmtowns.png", "./assets/music/fmtowns.ogg"},
                {"MSX", "assets/tilesets/msx.png", "./assets/music/msx.ogg"},
                {"Apple IIgs", "assets/tilesets/iigs.png", "./assets/music/iigs.ogg"},
                {"PC-98", "assets/tilesets/pc98.png", "./assets/music/pc98.ogg"},
                {"Atari", "assets/tilesets/atari.png", "./assets/music/atari.ogg"},
                {"VGA", "assets/tilesets/vga.png", kDefaultMusic},  // VGA=DOS 重染,無獨立 BGM
            };
            // config 指定的 tileset_png:在清單則用其 index;不在(自訂路徑)→ 附加成新 pack,
            // 避免靜默 fallback 到 pngPacks[0] 而載錯 tileset。
            auto pngPackPath = Configuration::getTilesetPng();
            int cfgPngIdx = -1;
            for (size_t i = 0; i < pngPacks.size(); ++i)
                if (pngPacks[i].tiles == pngPackPath) { cfgPngIdx = (int)i; break; }
            if (cfgPngIdx < 0) {
                pngPacks.push_back({I18n::t("tileset.custom"), pngPackPath, kDefaultMusic});
                cfgPngIdx = (int)pngPacks.size() - 1;
            }
            const int kBinModes = 2;  // EGA, CGA
            const char *kBinNames[] = {"EGA", "CGA"};
            int tilesetIdx = 0;
            {
                auto ts = Configuration::getTileset();
                if (ts == "cga") tilesetIdx = 1; else if (ts == "png") tilesetIdx = kBinModes + cfgPngIdx;
            }
            int tilesetCount = kBinModes + (int)pngPacks.size();
            auto tilesetName = [&](int idx) {
                return idx >= kBinModes ? pngPacks[idx - kBinModes].name : std::string(kBinNames[idx]);
            };
            // 音樂跟隨平台:平台 BGM 載入失敗(缺檔)→ 退回占位曲,不中斷。
            auto applyMusic = [&](int idx) {
                std::string m = idx >= kBinModes ? pngPacks[idx - kBinModes].music : kDefaultMusic;
                if (!Audio::playMusic(m)) Audio::playMusic(kDefaultMusic);
            };
            auto applyTileset = [&](int idx) {
                if (idx >= kBinModes)
                    overworldScreen->initFromPng(gRenderer, pngPacks[idx - kBinModes].tiles);
                else if (idx == 1)
                    overworldScreen->init(gRenderer, make_unique<CGALinearDecodeStrategy>(16, 16).get(), cgaTilesPath);
                else
                    overworldScreen->init(gRenderer, make_unique<EGARowPlanarDecodeStrategy>(16, 16).get(), egaTilesPath);
            };

            auto townScreen = make_shared<TownScreen>(gameContext, gRenderer);

            applyTileset(tilesetIdx);

            // 螢幕邏輯畫布 = 放大後 640x400;世界另畫進 320x200 離屏 target。
            SDL_RenderSetLogicalSize(gRenderer, CANVAS_W, CANVAS_H);
            gWorldTarget = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_RGBA8888,
                                             SDL_TEXTUREACCESS_TARGET, GAME_W, GAME_H);

            //Keeps track of time between steps
            LTimer stepTimer;
            Fonts::init(gRenderer);
            Audio::init();
            applyMusic(tilesetIdx);  // 初始音樂跟隨起始平台

            _playerStatusDisplay = make_shared<PlayerStatusDisplay>(gRenderer, player);
            _commandDisplay = make_shared<CommandDisplay>(gRenderer);
            SDL_Rect commandDisplayBox = {CommandDisplay::POSITION_X, CommandDisplay::POSITION_Y, CommandDisplay::WIDTH,
                                          CommandDisplay::HEIGHT};

            shared_ptr<Dungeon> dungeon = make_shared<Dungeon>("My pet dungeon");
            dungeon->randomize();
            dungeonScreen->setDungeon(dungeon);

            shared_ptr<Screen> currentScreen = static_pointer_cast<Screen>(overworldScreen);

            // ── 建角開場(新遊戲且非測試)──────────────────────────────
            // ↑↓ 選列、←→ 調整(性別/種族/職業循環,屬性加減 ←減 →加 受剩餘點數限制)、
            // Enter 在「開始」列確認 → 套到 player。關窗 → 直接離開。
            if (needCharCreate) {
                CharCreateState cc;
                bool ccDone = false, ccQuit = false;
                while (!ccDone && !ccQuit) {
                    SDL_Event ce;
                    while (SDL_PollEvent(&ce) != 0) {
                        if (ce.type == SDL_QUIT) { ccQuit = true; break; }
                        if (ce.type != SDL_KEYDOWN) continue;
                        auto k = ce.key.keysym.sym;
                        if (k == SDLK_UP)   cc.row = (cc.row + kCCRowN - 1) % kCCRowN;
                        else if (k == SDLK_DOWN) cc.row = (cc.row + 1) % kCCRowN;
                        else if (k == SDLK_LEFT || k == SDLK_RIGHT) {
                            int d = (k == SDLK_RIGHT) ? 1 : -1;
                            if (cc.row == 0) cc.sex = (cc.sex + CharCreation::kSexN + d) % CharCreation::kSexN;
                            else if (cc.row == 1) cc.raceIdx = (cc.raceIdx + CharCreation::kRaceN + d) % CharCreation::kRaceN;
                            else if (cc.row == 2) cc.klassIdx = (cc.klassIdx + CharCreation::kKlassN + d) % CharCreation::kKlassN;
                            else if (cc.row >= 3 && cc.row < kCCRowStart) {
                                int ai = cc.row - 3;
                                if (d > 0 && cc.remaining() > 0) cc.alloc[ai]++;
                                else if (d < 0 && cc.alloc[ai] > 0) cc.alloc[ai]--;
                            }
                        } else if (k == SDLK_RETURN) {
                            if (cc.row == kCCRowStart) ccDone = true;
                            else cc.row = kCCRowStart;  // 任意列按 Enter → 跳到開始,再按一次確認
                        }
                    }
                    SDL_SetRenderTarget(gRenderer, nullptr);
                    SDL_RenderSetLogicalSize(gRenderer, CANVAS_W, CANVAS_H);
                    drawCharCreate(gRenderer, cc);
                    SDL_RenderPresent(gRenderer);
                    SDL_Delay(16);
                }
                if (ccQuit) { close(); return 0; }
                CharCreation::apply(*player, cc.sex, cc.raceIdx, cc.klassIdx, cc.alloc);
            }

            //While application is running
            bool quitDialogActive = false;
            bool settingsActive = false;
            bool helpActive = false;
            int shopMode = 0;            // 0=關、1=類別、2=品項
            ShopType shopType = ShopType::Weapon;
            int shopSel = 0;
            bool kingActive = false;     // 城堡國王對話
            bool castActive = false;     // 地牢施法選單
            int castSel = 0;
            bool ztatsActive = false;    // Z 角色屬性表
            bool victoryActive = false;  // 終局勝利畫面
            int settingsRow = 0;
            while (!quit) {
                //Handle events on queue
                while (SDL_PollEvent(&e) != 0) {
                    // 商店開啟時:↑↓選、Enter 確定/購買、ESC 返回/關閉,吞掉其他輸入
                    if (shopMode > 0) {
                        if (e.type == SDL_KEYDOWN) {
                            auto k = e.key.keysym.sym;
                            int n = (shopMode == 1) ? kShopCatN : (int)ItemCatalog::itemsFor(shopType).size();
                            if (k == SDLK_ESCAPE) { if (shopMode == 2) { shopMode = 1; shopSel = 0; } else shopMode = 0; }
                            else if (k == SDLK_UP) shopSel = (shopSel + n - 1) % n;
                            else if (k == SDLK_DOWN) shopSel = (shopSel + 1) % n;
                            else if (k == SDLK_RETURN) {
                                if (shopMode == 1) { shopType = kShopCats[shopSel]; shopMode = 2; shopSel = 0; }
                                else {
                                    auto items = ItemCatalog::itemsFor(shopType);
                                    auto &it = items[shopSel];
                                    if (player->getMoney() < it.price) {
                                        CommandDisplay::writeLn(I18n::t("shop.no_gold"), false);
                                    } else {
                                        player->addMoney(-it.price);
                                        if (shopType == ShopType::Food) {
                                            player->addFood(50);
                                            CommandDisplay::writeLn(I18n::t("shop.bought_food"), false);
                                        } else if (shopType == ShopType::Weapon) {
                                            player->addWeapon(it.index);
                                            if (it.index > player->getCurrentWeapon()) player->setCurrentWeapon(it.index);
                                            CommandDisplay::writeLn(I18n::tf("shop.bought", {I18n::t(it.nameKey)}), false);
                                        } else if (shopType == ShopType::Armor) {
                                            player->addArmor(it.index);
                                            if (it.index > player->getCurrentArmor()) player->setCurrentArmor(it.index);
                                            CommandDisplay::writeLn(I18n::tf("shop.bought", {I18n::t(it.nameKey)}), false);
                                        } else if (shopType == ShopType::Pub) {
                                            CommandDisplay::writeLn(I18n::t("clue." + to_string(rand() % 6)), false);
                                        } else if (shopType == ShopType::Transport) {
                                            player->setRaft(true);
                                            CommandDisplay::writeLn(I18n::t("shop.bought_raft"), false);
                                        } else {  // Magic
                                            player->addSpell(it.index);
                                            CommandDisplay::writeLn(I18n::tf("shop.bought", {I18n::t(it.nameKey)}), false);
                                        }
                                        Audio::playSfx("./assets/sfx/select.ogg");
                                    }
                                }
                            }
                        }
                        continue;
                    }

                    // 施法選單開啟時:↑↓選、Enter 施放、ESC 關閉
                    if (castActive) {
                        if (e.type == SDL_KEYDOWN) {
                            auto k = e.key.keysym.sym;
                            auto owned = ownedSpells(*player);
                            if (k == SDLK_ESCAPE) castActive = false;
                            else if (!owned.empty() && k == SDLK_UP) castSel = (castSel + (int)owned.size() - 1) % owned.size();
                            else if (!owned.empty() && k == SDLK_DOWN) castSel = (castSel + 1) % owned.size();
                            else if (k == SDLK_RETURN && !owned.empty()) {
                                dungeonScreen->castSpell(owned[castSel]);
                                Audio::playSfx("./assets/sfx/select.ogg");
                                castActive = false;
                            }
                        }
                        continue;
                    }

                    // 國王對話開啟時:Enter 接受任務 / 領賞、ESC 關閉
                    if (kingActive) {
                        if (e.type == SDL_KEYDOWN) {
                            auto k = e.key.keysym.sym;
                            if (k == SDLK_ESCAPE) kingActive = false;
                            else if (k == SDLK_RETURN) {
                                if (endgameReady(*player)) {
                                    kingActive = false; victoryActive = true;
                                } else if (!player->hasQuest()) {
                                    int tgt = questTargetFor(*player);
                                    player->giveQuest(tgt);
                                    CommandDisplay::writeLn(I18n::tf("king.accepted", {to_string(tgt)}), false);
                                    kingActive = false;
                                } else if (player->isQuestComplete()) {
                                    int g = questGoldFor(*player);
                                    player->addMoney(g);
                                    player->setStrength(player->getStrength() + 1);
                                    player->completeQuest();
                                    CommandDisplay::writeLn(I18n::tf("king.reward", {to_string(g)}), false);
                                    Audio::playSfx("./assets/sfx/select.ogg");
                                    kingActive = false;
                                }
                            }
                        }
                        continue;
                    }

                    // 勝利畫面 / 角色屬性表 / 說明畫面:任意鍵關閉
                    if (victoryActive) {
                        if (e.type == SDL_KEYDOWN) victoryActive = false;
                        continue;
                    }
                    if (ztatsActive) {
                        if (e.type == SDL_KEYDOWN) ztatsActive = false;
                        continue;
                    }
                    if (helpActive) {
                        if (e.type == SDL_KEYDOWN) helpActive = false;
                        continue;
                    }

                    // F6 設定選單開啟時:↑↓選列(3 列)、←→ 調整 / 切換、F6/ESC 關閉,吞掉其他輸入
                    if (settingsActive) {
                        if (e.type == SDL_KEYDOWN) {
                            auto k = e.key.keysym.sym;
                            if (k == SDLK_F6 || k == SDLK_ESCAPE) settingsActive = false;
                            else if (k == SDLK_UP) settingsRow = (settingsRow + 2) % 3;
                            else if (k == SDLK_DOWN) settingsRow = (settingsRow + 1) % 3;
                            else if (k == SDLK_LEFT || k == SDLK_RIGHT) {
                                if (settingsRow == 2) {
                                    Configuration::setChaseMonsters(!Configuration::getChaseMonsters());  // 開關
                                } else {
                                    int d = (k == SDLK_RIGHT) ? 5 : -5;
                                    if (settingsRow == 0) Configuration::setSpeedPct(Configuration::getSpeedPct() + d);
                                    else Configuration::setSpawnPct(Configuration::getSpawnPct() + d);
                                }
                            }
                        }
                        continue;
                    }

                    // 離開語意鐵則:ESC 只取消,F10/Ctrl+Q(及關窗)才彈離開確認
                    if (quitDialogActive) {
                        if (e.type == SDL_KEYDOWN) {
                            auto k = e.key.keysym.sym;
                            if (k == SDLK_y || k == SDLK_RETURN) {
                                // 離開鐵則:離開前 autosave(失敗則提示,但仍允許離開)
                                if (!SaveGame::save(*player, SaveGame::defaultPath()))
                                    CommandDisplay::writeLn(I18n::t("msg.save_fail_exit"), false);
                                quit = true;
                            } else if (k == SDLK_n || k == SDLK_ESCAPE) {
                                quitDialogActive = false;
                            }
                        }
                        continue;  // dialog 開啟時吞掉其他輸入
                    }

                    //User requests quit
                    if (e.type == SDL_QUIT) {
                        quitDialogActive = true;  // 關窗也走確認流程
                    } else {
                        if (e.type == SDL_KEYDOWN) {
                            auto pressedKey = e.key.keysym.sym;
                            bool ctrl = (e.key.keysym.mod & KMOD_CTRL) != 0;
                            if (pressedKey == SDLK_F10 || (ctrl && pressedKey == SDLK_q)) {
                                quitDialogActive = true;
                                continue;
                            }
                            if (pressedKey == SDLK_F6) {
                                settingsActive = true;
                                continue;
                            }
                            // F1:說明畫面(列出所有指令)。
                            if (pressedKey == SDLK_F1) {
                                helpActive = true;
                                continue;
                            }
                            // Z:角色屬性表(Ztats)
                            if (pressedKey == SDLK_z) {
                                ztatsActive = true;
                                continue;
                            }
                            // PageDown:循環切換 tileset(EGA→CGA→各平台 PNG 包)+ 中文提示。
                            if (pressedKey == SDLK_PAGEDOWN) {
                                tilesetIdx = (tilesetIdx + 1) % tilesetCount;
                                applyTileset(tilesetIdx);
                                applyMusic(tilesetIdx);  // 音樂跟隨切換的平台
                                CommandDisplay::writeLn(string(I18n::t("msg.graphics_mode")) + tilesetName(tilesetIdx), false);
                            }
                            // M:切換背景音樂
                            if (pressedKey == SDLK_m) {
                                if (Audio::available()) {
                                    bool on = Audio::toggleMute();
                                    CommandDisplay::writeLn(on ? I18n::t("msg.music_on") : I18n::t("msg.music_off"), false);
                                } else {
                                    CommandDisplay::writeLn(I18n::t("msg.music_no_device"), false);
                                }
                            }
                            // B:在城鎮/城堡開商店(交易)
                            if (pressedKey == SDLK_b &&
                                gameContext->getCurrentScreen() == ScreenType::Town) {
                                shopMode = 1; shopSel = 0;
                                continue;
                            }
                            // T:在城堡晉見國王(任務)
                            if (pressedKey == SDLK_t &&
                                gameContext->getCurrentScreen() == ScreenType::Town &&
                                gameContext->isInCastle()) {
                                kingActive = true;
                                continue;
                            }
                            // C:地牢施法
                            if (pressedKey == SDLK_c &&
                                gameContext->getCurrentScreen() == ScreenType::Dungeon) {
                                castActive = true; castSel = 0;
                                continue;
                            }
                            // F5:手動存檔
                            if (pressedKey == SDLK_F5) {
                                bool ok = SaveGame::save(*player, SaveGame::defaultPath());
                                CommandDisplay::writeLn(ok ? I18n::t("msg.saved") : I18n::t("msg.save_fail"), false);
                            }
                            // F9:切換音效(獨立於音樂)
                            if (pressedKey == SDLK_F9) {
                                if (Audio::available()) {
                                    bool on = Audio::toggleSfx();
                                    CommandDisplay::writeLn(on ? I18n::t("msg.sfx_on") : I18n::t("msg.sfx_off"), false);
                                } else {
                                    CommandDisplay::writeLn(I18n::t("msg.sfx_no_device"), false);
                                }
                            }
                            // 音效:移動踏步 / 攻擊撞擊(音效靜音時自動 no-op)
                            if (pressedKey == SDLK_UP || pressedKey == SDLK_DOWN ||
                                pressedKey == SDLK_LEFT || pressedKey == SDLK_RIGHT)
                                Audio::playSfx("./assets/sfx/step.ogg");
                            else if (pressedKey == SDLK_a)
                                Audio::playSfx("./assets/sfx/bump.ogg");
                        }

                        currentScreen->handle(e);
                    }
                }

                //Calculate time step
                float timeStep = stepTimer.getTicks();

                //Restart step timer
                stepTimer.start();

                // === 世界繪製:畫進原生 320x200 離屏 target ===
                SDL_SetRenderTarget(gRenderer, gWorldTarget);
                SDL_RenderSetLogicalSize(gRenderer, GAME_W, GAME_H);
                SDL_SetRenderDrawColor(gRenderer, 0, 0, 0xAF, 0);
                SDL_RenderClear(gRenderer);

                switch (gameContext->getCurrentScreen()) {
                    case ScreenType::Overworld:
                        currentScreen = static_pointer_cast<Screen>(overworldScreen);
                        break;
                    case ScreenType::Dungeon:
                        currentScreen = static_pointer_cast<Screen>(dungeonScreen);
                        break;
                    case ScreenType::Town:
                        if (currentScreen != townScreen) {
                            townScreen->refresh();
                        }
                        currentScreen = static_pointer_cast<Screen>(townScreen);
                        break;
                    case ScreenType::Space:
                        throw runtime_error("Unhandled");
                }

                currentScreen->update(timeStep);

                SDL_Rect defaultViewport = {0, 0, GAME_W, GAME_H};
                SDL_RenderSetViewport(gRenderer, &defaultViewport);

                currentScreen->draw(gRenderer);

                // === 放大世界 target 到螢幕 640x400 (nearest, crisp) ===
                SDL_SetRenderTarget(gRenderer, nullptr);
                SDL_RenderSetLogicalSize(gRenderer, CANVAS_W, CANVAS_H);
                SDL_RenderSetViewport(gRenderer, nullptr);
                SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 0xFF);
                SDL_RenderClear(gRenderer);
                SDL_RenderCopy(gRenderer, gWorldTarget, nullptr, nullptr);

                // 地牢層數/朝向:在 640 高解析層用 cjkUi(16px)繪製,複雜漢字(層)才不會糊掉。
                if (currentScreen.get() == dungeonScreen.get()) {
                    SDL_Color teal{0x42, 0xFF, 0xFF, 0xFF};
                    int cx = 300;  // 對齊地牢線框中心(世界 target 區域)
                    LTexture lv;
                    lv.loadFromRenderedText(Fonts::cjkUi(), gRenderer,
                        I18n::tf("dg.level", {to_string(player->getDungeonLevel() + 1)}), teal);
                    lv.render(gRenderer, cx - lv.getWidth() / 2, 2);
                    LTexture ori;
                    ori.loadFromRenderedText(Fonts::cjkUi(), gRenderer,
                        CardinalPointUtils::toString(player->getDungeonOrientation()), teal);
                    ori.render(gRenderer, cx - ori.getWidth() / 2, 298);
                }

                // === UI(中文)在 640x400 高解析空間繪製,座標 x2 ===
                _playerStatusDisplay->draw(gRenderer);

                SDL_Rect commandBoxHi = {commandDisplayBox.x * CANVAS_SCALE, commandDisplayBox.y * CANVAS_SCALE,
                                         commandDisplayBox.w * CANVAS_SCALE, commandDisplayBox.h * CANVAS_SCALE};
                SDL_RenderSetViewport(gRenderer, &commandBoxHi);
                _commandDisplay->draw(gRenderer);

                // 設定選單 / 離開確認對話框(覆蓋最上層)
                if (settingsActive) {
                    SDL_RenderSetViewport(gRenderer, nullptr);
                    drawSettingsMenu(gRenderer, settingsRow);
                }
                if (quitDialogActive) {
                    SDL_RenderSetViewport(gRenderer, nullptr);
                    drawQuitDialog(gRenderer);
                }
                if (helpActive) {
                    SDL_RenderSetViewport(gRenderer, nullptr);
                    drawHelpScreen(gRenderer);
                }
                if (shopMode > 0) {
                    SDL_RenderSetViewport(gRenderer, nullptr);
                    drawShop(gRenderer, *player, shopMode, shopType, shopSel);
                }
                if (kingActive) {
                    SDL_RenderSetViewport(gRenderer, nullptr);
                    drawKing(gRenderer, *player);
                }
                if (castActive) {
                    SDL_RenderSetViewport(gRenderer, nullptr);
                    drawCast(gRenderer, *player, castSel);
                }
                if (ztatsActive) {
                    SDL_RenderSetViewport(gRenderer, nullptr);
                    drawZtats(gRenderer, *player);
                }
                if (victoryActive) {
                    SDL_RenderSetViewport(gRenderer, nullptr);
                    drawVictory(gRenderer);
                }

                //Update screen
                SDL_RenderPresent(gRenderer);
            }
        }
    }

    //Free resources and close SDL
    close();

    return 0;
}