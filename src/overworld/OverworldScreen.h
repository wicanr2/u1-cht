#pragma once

#include "../GameObject.h"
#include <utility>
#include <vector>
#include "OverworldTile.h"
#include "OverworldEnemy.h"
#include "OverworldSpriteType.h"
#include "../Player.h"
#include "Constants.h"
#include <functional>
#include "../PlayerStatusDisplay.h"
#include <map>
#include "../GameContext.h"
#include "../Screen.h"

using namespace std;

class OverworldScreen : public Screen {
public:
    static constexpr int DISPLAY_SIZE_TILES_WIDTH = 19;
    static constexpr int DISPLAY_SIZE_TILES_HEIGHT = 9;
    static constexpr int MAP_WIDTH_PX = DISPLAY_SIZE_TILES_WIDTH * TILE_WIDTH;
    static constexpr int MAP_HEIGHT_PX = DISPLAY_SIZE_TILES_HEIGHT * TILE_WIDTH;

    OverworldScreen(shared_ptr<GameContext> context, int widthTiles, int heightTiles) : Screen(std::move(context)) {
        _camera.w = DISPLAY_SIZE_TILES_WIDTH * TILE_WIDTH;
        _camera.h = DISPLAY_SIZE_TILES_HEIGHT * TILE_HEIGHT;

        setCamera();
    };

    void init(SDL_Renderer *renderer, PixelDecodeStrategy *pixelDecodeStrategy, const string &tilesFsPath);

    // PNG AssetPack:從 PNG sprite sheet 載入 tileset(跨平台素材包)
    void initFromPng(SDL_Renderer *renderer, const string &pngPath);

    void update(float elapsed) override;

    void draw(SDL_Renderer *renderer) override;

    void handle(const SDL_Event &event) override;

    // 騎乘時把玩家圖示換成載具 sprite(徒步→PLAYER);買載具後 / 進世界時呼叫
    void refreshPlayerVehicleSprite();

private:
    static constexpr int BOUND_X_TILES = 167;
    static constexpr int BOUND_Y_TILES = 155;
    static constexpr int MAP_FILE_SIZE = 13103;
    static constexpr int TILES_PER_ROW = 168;

    // 怪物上限(同時在場);turn-based 時間 tick 累加器(參考 u2-cht)
    static constexpr int MOB_MAX = 8;
    int _timeAccum = 0;

    static OverworldSpriteType::SpriteType getSpriteType(int tileTypeId);

    void buildFromSprites(const vector<shared_ptr<OverworldSpriteType>> &spriteTypes, SDL_Renderer *renderer);

    shared_ptr<OverworldTile> _playerTile;
    vector<shared_ptr<OverworldTile>> _tiles;
    SDL_Rect _camera;
    map<OverworldSpriteType::SpriteType, shared_ptr<OverworldSpriteType>> _spritesMap;

    vector<shared_ptr<OverworldEnemy>> _enemies;
    bool _attackMode = false;
    bool _horseStepping = false;   // 馬雙步防遞迴

    void setCamera();

    static int toPixels(int tiles);

    static int toTiles(int pixels);

    void executeOnVisibleTiles(const function<void(OverworldTile *)> &);

    void moveOrAttack(int deltaX, int deltaY);

    void move(int deltaX, int deltaY);

    void attack(int deltaX, int deltaY);

    void enterPlace();

    static int getTileOffsetFromPositionInPixels(int xPx, int yPx);

    static int getTileOffset(int x, int y);

    void activateNpcs();

    void onStep();      // 每走一步:推進時間 tick(speed_pct)+ 嘗試生怪(spawn_pct)

    // 地面怪「相鄰時反擊玩家」(忠於原版 U1:地面怪不移動,只在相鄰時攻擊)
    void overworldMonsterTurn();

    void respawnPlayer();   // 死亡復活(回城堡、HP 回滿、損失半數金幣)

    void spawnNpcs();

    bool isPassable(int x, int y);

    void setAttackMode(bool set);

    static string getCardinalPointFromDeltas(int deltaX, int deltaY);
};

