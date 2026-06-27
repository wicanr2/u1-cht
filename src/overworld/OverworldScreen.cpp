#include "OverworldScreen.h"
#include "../common/I18n.h"
#include "../Combat.h"
#include "TileTypeLoader.h"
#include "../common/graphics/PixelDecodeStrategy.h"
#include "Constants.h"
#include "TileAnimation.h"
#include "../common/Fonts.h"
#include "../CommandDisplay.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include "../common/ShapeUtils.h"
#include "../Configuration.h"

void OverworldScreen::init(SDL_Renderer *renderer,
                           PixelDecodeStrategy *pixelDecodeStrategy, const string &tilesFsPath) {
    // load all sprite types (BIN-decode 路徑).
    auto spriteTypes = TileTypeLoader::loadOverworldSprites(tilesFsPath, pixelDecodeStrategy, renderer);
    buildFromSprites(spriteTypes, renderer);
}

// PNG AssetPack 路徑:從 PNG sprite sheet 載入 tileset(跨平台素材包)
void OverworldScreen::initFromPng(SDL_Renderer *renderer, const string &pngPath) {
    auto spriteTypes = TileTypeLoader::loadOverworldSpritesFromPng(pngPath, renderer);
    buildFromSprites(spriteTypes, renderer);
}

void OverworldScreen::buildFromSprites(const vector<shared_ptr<OverworldSpriteType>> &spriteTypes,
                                       SDL_Renderer *renderer) {
    _tiles.clear();
    _spritesMap.clear();

    // write to map so it's easier to find the sprite type by type name.
    for (const auto &spriteType : spriteTypes) {
        _spritesMap.insert(pair<OverworldSpriteType::SpriteType, shared_ptr<OverworldSpriteType>>(
                spriteType->getType(), spriteType));
    }

    string mapFileLocation = Configuration::getMapFilePath();

    auto file = SDL_RWFromFile(mapFileLocation.c_str(), "r+b");
    auto buffer = new uint8_t[MAP_FILE_SIZE];
    SDL_RWread(file, buffer, 1, MAP_FILE_SIZE);
    SDL_RWclose(file);

    auto currentX = -1;
    auto currentY = 0;

    // All water and static tiles share the same animation (water needs to scroll at the same time, and NOP tiles don't do anything)
    auto defaultSharedAnimation = make_shared<TileAnimation>();

    // map file format is one byte contains the data of two tiles, one per nibble, so shift and mask them.
    for (int i = 0; i < MAP_FILE_SIZE; i++) {
        auto byte = buffer[i];
        auto idTileNibble1 = byte >> 4;
        auto idTileNibble2 = byte & 0b00001111;
        auto spriteType1 = getSpriteType(idTileNibble1);
        auto spriteType2 = getSpriteType(idTileNibble2);
        auto sprite1 = _spritesMap.find(spriteType1)->second;
        auto sprite2 = _spritesMap.find(spriteType2)->second;

        currentX++;
        if (currentX > BOUND_X_TILES) {
            currentX = 0;
            currentY++;
        }
        auto x1 = currentX;
        auto y1 = currentY;

        currentX++;
        if (currentX > BOUND_X_TILES) {
            currentX = 0;
            currentY++;
        }
        auto x2 = currentX;
        auto y2 = currentY;

        // water tiles share the same scrolling animation instance so they all animate at the same time
        auto animation = sprite1->getAnimationType() == TileAnimation::AnimationType::SCROLLING ? defaultSharedAnimation
                                                                                                : make_shared<TileAnimation>();
        auto tile1 = make_shared<OverworldTile>(toPixels(x1), toPixels(y1), sprite1, animation);
        _tiles.push_back(tile1);

        if (sprite1->getType() == OverworldSpriteType::SpriteType::TOWN) {
            cout << "Town at: " << x1 << "," << y1 << "\n";
        } else if (sprite1->getType() == OverworldSpriteType::SpriteType::CASTLE) {
            cout << "Castle at: " << x1 << "," << y1 << "\n";
        }

        animation = sprite2->getAnimationType() == TileAnimation::AnimationType::SCROLLING ? defaultSharedAnimation
                                                                                           : make_shared<TileAnimation>();
        auto tile2 = make_shared<OverworldTile>(toPixels(x2), toPixels(y2), sprite2, animation);
        _tiles.push_back(tile2);

        if (sprite2->getType() == OverworldSpriteType::SpriteType::TOWN) {
            cout << "Town at: " << x2 << "," << y2 << "\n";
        } else if (sprite2->getType() == OverworldSpriteType::SpriteType::CASTLE) {
            cout << "Castle at: " << x2 << "," << y2 << "\n";
        }
    }

    _playerTile = make_shared<OverworldTile>(toPixels(_gameContext->getPlayer()->getOverworldX()),
                                             toPixels(_gameContext->getPlayer()->getOverworldY()),
                                             _spritesMap.find(OverworldSpriteType::SpriteType::PLAYER)->second,
                                             defaultSharedAnimation);

    // 切換 tileset 後:把現存 NPC 的 sprite 重新指向新 tileset 的同型 sprite。
    // 否則 NPC 會保留舊 tileset 外觀(palette 不一致),且一旦 sprite 來源失效就會消失。
    for (const auto &enemy : _enemies) {
        auto it = _spritesMap.find(enemy->getTile()->getSpriteType());
        if (it != _spritesMap.end()) {
            enemy->getTile()->setSprite(it->second);
        }
    }

    delete[] buffer;
}

void OverworldScreen::update(float elapsed) {
    // scrolling animation (water) needs to be animated on its own.
    TileAnimation::updateScrolling(elapsed);

    executeOnVisibleTiles([elapsed](OverworldTile *tile) -> void { tile->update(elapsed); });

    for (const auto &enemy: _enemies) {
        enemy->update(elapsed);
    }
    // 生怪改為 turn-based(每步觸發,見 onStep),不再用 real-time 計時。
}

void OverworldScreen::draw(SDL_Renderer *renderer) {
    ShapeUtils::drawFullBorders(renderer);

    SDL_Rect mapViewPort = {Screen::MAIN_VIEWPORT_PADDING, Screen::MAIN_VIEWPORT_PADDING,
                            Screen::WIDTH, Screen::HEIGHT};
    SDL_RenderSetViewport(renderer, &mapViewPort);

    // why do i need to declare a variable for a capture? fuck!
    auto camera = _camera;

    // this is a lambda expression, and i thought java was verbose!
    executeOnVisibleTiles([renderer, camera](Tile *tile) -> void { tile->draw(renderer, camera); });

    _playerTile->draw(renderer, camera);

    for (const auto &enemy: _enemies) {
        enemy->draw(renderer, camera);
    }
}

void OverworldScreen::moveOrAttack(int deltaX, int deltaY) {
    if (!_attackMode) {
        move(deltaX, deltaY);
    } else {
        attack(deltaX, deltaY);
    }
}


void OverworldScreen::move(int deltaX, int deltaY) {
    int playerX = _gameContext->getPlayer()->getOverworldX() + deltaX;
    int playerY = _gameContext->getPlayer()->getOverworldY() + deltaY;

    // don't allow the player to move outside of overworld bounds (should this game allow world map wrapping?)
    if (playerX < 0) playerX = 0;
    if (playerX > BOUND_X_TILES) playerX = BOUND_X_TILES;
    if (playerY < 0) playerY = 0;
    if (playerY > BOUND_Y_TILES) playerY = BOUND_Y_TILES;

    // Don't allow player to walk across mountains or through water.
    OverworldSpriteType::SpriteType typeTypeSteppedOn = _tiles[getTileOffset(playerX, playerY)]->getSpriteType();
    if (typeTypeSteppedOn == OverworldSpriteType::SpriteType::MOUNTAIN) {
        CommandDisplay::writeLn(I18n::t("ow.mountain_blocked"), true);
        return;
    }

    if (typeTypeSteppedOn == OverworldSpriteType::SpriteType::WATER) {
        if (!_gameContext->getPlayer()->hasRaft()) {
            CommandDisplay::writeLn(I18n::t("ow.water_blocked"), true);
            return;
        }
        CommandDisplay::writeLn(I18n::t("ow.sailing"), true);   // 有筏 → 揚帆渡水
    }

    // Don't allow player to walk past an enemy
    for (const auto &enemy: _enemies) {
        if (enemy->getX() == playerX && enemy->getY() == playerY) {
            CommandDisplay::writeLn(I18n::tf("ow.blocked_by", {enemy->getName()}), true);
            return;
        }
    }

    CommandDisplay::writeLn(getCardinalPointFromDeltas(deltaX, deltaY), true);

    _gameContext->getPlayer()->setOverworldX(playerX);
    _gameContext->getPlayer()->setOverworldY(playerY);

    _playerTile->setCoordinates(toPixels(playerX), toPixels(playerY));

    cout << "\nMoved to: " << playerX << ", " << playerY << "\n";

    // re-center camera on player if possible
    setCamera();

    // 成功走一步 → 推進 turn-based 時間 tick + 嘗試生怪
    onStep();
}

void OverworldScreen::attack(int deltaX, int deltaY) {
    auto attackX = _gameContext->getPlayer()->getOverworldX() + deltaX;
    auto attackY = _gameContext->getPlayer()->getOverworldY() + deltaY;

    shared_ptr<OverworldEnemy> target;
    int index = 0;
    for (const auto &enemy: _enemies) {
        if (enemy->getX() == attackX && enemy->getY() == attackY) {
            target = enemy;
            break;
        }
        index++;
    }

    CommandDisplay::writeLn(getCardinalPointFromDeltas(deltaX, deltaY), true);
    _attackMode = false;

    if (!target) {
        CommandDisplay::writeLn(I18n::t("ow.no_target"), false);
        return;
    }
    auto player = _gameContext->getPlayer();
    int dmg = Combat::playerAttackDamage(*player);
    target->receiveDamage(dmg);
    if (target->isDead()) {
        player->gainXP(Combat::killXP(10));
        player->recordKill();
        CommandDisplay::writeLn(I18n::tf("ow.killed", {target->getName()}), false);
        _enemies.erase(_enemies.begin() + index, _enemies.begin() + index + 1);
    } else {
        CommandDisplay::writeLn(I18n::tf("ow.hit", {to_string(dmg)}), false);
    }
}

void OverworldScreen::enterPlace() {
    // What's on the player's tile?
    int x = _gameContext->getPlayer()->getOverworldX();
    int y = _gameContext->getPlayer()->getOverworldY();

    auto currentTile = _tiles[getTileOffset(x, y)];

    auto currentTileType = currentTile->getSpriteType();

    switch (currentTileType) {
        case OverworldSpriteType::SpriteType::CASTLE:
            _gameContext->enterCastle();
            break;
        case OverworldSpriteType::SpriteType::TOWN:
            _gameContext->enterTown();
            break;
        case OverworldSpriteType::SpriteType::DUNGEON_ENTRANCE:
            _gameContext->setScreen(ScreenType::Dungeon);
            break;
        default:
            break;
    }
}

void OverworldScreen::handle(const SDL_Event &event) {
    if (event.type == SDL_KEYDOWN) {
        auto pressedKey = event.key.keysym.sym;

        switch (pressedKey) {
            case SDLK_UP:
                moveOrAttack(0, -1);
                break;
            case SDLK_DOWN:
                moveOrAttack(0, 1);
                break;
            case SDLK_LEFT:
                moveOrAttack(-1, 0);
                break;
            case SDLK_RIGHT:
                moveOrAttack(1, 0);
                break;
            case SDLK_e:
                enterPlace();
                break;
            case SDLK_a:
                setAttackMode(true);
                break;
            default:
                setAttackMode(false);
                return;
        }
    }

    activateNpcs();
}

void OverworldScreen::setCamera() {
    _camera.x = toPixels(_gameContext->getPlayer()->getOverworldX() - (DISPLAY_SIZE_TILES_WIDTH - 1) / 2);
    _camera.y = toPixels(_gameContext->getPlayer()->getOverworldY() - (DISPLAY_SIZE_TILES_HEIGHT - 1) / 2);

    int cameraBoundX = toPixels(BOUND_X_TILES);
    int cameraBoundY = toPixels(BOUND_Y_TILES);

    // make sure the camera doesn't show where the world ends!
    if (_camera.x < 0) _camera.x = 0;
    if (_camera.y < 0) _camera.y = 0;
    if (_camera.x + toPixels(DISPLAY_SIZE_TILES_WIDTH) > cameraBoundX)
        _camera.x = cameraBoundX -
                    toPixels(DISPLAY_SIZE_TILES_WIDTH);
    if (_camera.y + toPixels(DISPLAY_SIZE_TILES_HEIGHT) > cameraBoundY)
        _camera.y = cameraBoundY -
                    toPixels(DISPLAY_SIZE_TILES_HEIGHT);
}

int OverworldScreen::toPixels(int tiles) {
    return tiles * TILE_WIDTH;
}

int OverworldScreen::toTiles(int pixels) {
    return pixels / TILE_WIDTH;
}

void OverworldScreen::executeOnVisibleTiles(const function<void(OverworldTile *)> &func) {
    int tileStartOffset = getTileOffsetFromPositionInPixels(_camera.x, _camera.y);
    int offset = tileStartOffset;

    for (int y = 0; y < DISPLAY_SIZE_TILES_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_SIZE_TILES_WIDTH; x++) {
            int tileOffset = (offset + x);
            if (tileOffset >= _tiles.size()) continue;

            auto tile = _tiles[tileOffset];
            func(tile.get());
        }
        offset += TILES_PER_ROW;
    }
}

OverworldSpriteType::SpriteType OverworldScreen::getSpriteType(int tileTypeId) {
    switch (tileTypeId) {
        case 0: {
            return OverworldSpriteType::SpriteType::WATER;
        }
        case 1: {
            return OverworldSpriteType::SpriteType::GRASS;
        }
        case 2: {
            return OverworldSpriteType::SpriteType::FOREST;
        }
        case 3: {
            return OverworldSpriteType::SpriteType::MOUNTAIN;
        }
        case 4: {
            return OverworldSpriteType::SpriteType::CASTLE;
        }
        case 5: {
            return OverworldSpriteType::SpriteType::SIGNPOST;
        }
        case 6: {
            return OverworldSpriteType::SpriteType::TOWN;
        }
        case 7: {
            return OverworldSpriteType::SpriteType::DUNGEON_ENTRANCE;
        }
        default:
            throw runtime_error("Unknown sprite type");
    }
}

int OverworldScreen::getTileOffsetFromPositionInPixels(int xPx, int yPx) {
    return getTileOffset(toTiles(xPx), toTiles(yPx));
}

int OverworldScreen::getTileOffset(int x, int y) {
    return y * TILES_PER_ROW + x;
}

void OverworldScreen::activateNpcs() {
    auto player = _gameContext->getPlayer();

    for (auto enemy: _enemies) {
        auto diffX = enemy->getX() - player->getOverworldX();
        auto diffY = enemy->getY() - player->getOverworldY();

        if (abs(diffX) > abs(diffY)) {
            // enemy is farther away in the x-axis
            if (diffX < 0) {
                // player is to the right of the enemy

            } else {
                // player is to the left of the enemy
            }
        } else {
            // enemy is farther away in the y-axis
            if (diffY < 0) {
                // player is "below" of the enemy
            } else {
                // player is "above" the enemy
            }
        }
    }
}

// 地面格可否生怪/通行(界內、非山非水)
bool OverworldScreen::isPassable(int x, int y) {
    if (x < 0 || x > BOUND_X_TILES || y < 0 || y > BOUND_Y_TILES) return false;
    auto t = _tiles[getTileOffset(x, y)]->getSpriteType();
    return t != OverworldSpriteType::SpriteType::MOUNTAIN &&
           t != OverworldSpriteType::SpriteType::WATER;
}

// 每走一步:嘗試生怪(spawn_pct)+ 推進時間 tick(speed_pct)。參考 u2-cht。
void OverworldScreen::onStep() {
    spawnNpcs();

    // 時間累加器:滿 100 走一個時間 tick。地面怪「相鄰反擊」掛在 tick 上
    // (忠於原版 U1:地面怪不移動;移動追擊是地牢的行為,見 docs/ultima1-original-ai.md)。
    _timeAccum += Configuration::getSpeedPct();
    while (_timeAccum >= 100) {
        _timeAccum -= 100;
        overworldMonsterTurn();

        // 食物消耗 / 飢餓(每時間 tick;參考 u2-cht)
        auto pl = _gameContext->getPlayer();
        pl->consumeFood(1);
        if (pl->isStarving()) {
            pl->receiveDamage(2);
            CommandDisplay::writeLn(I18n::t("common.starving"), false);
            if (pl->isDead()) { CommandDisplay::writeLn(I18n::t("common.starved"), false); respawnPlayer(); }
        }
    }
}

// 地面怪行動:不移動(忠於原版 U1),僅在與玩家相鄰(曼哈頓距離=1)時攻擊。
void OverworldScreen::overworldMonsterTurn() {
    auto player = _gameContext->getPlayer();
    if (player->isDead()) return;

    int px = player->getOverworldX();
    int py = player->getOverworldY();

    // 追蹤模式(F6 開關,預設關):每隻怪朝玩家走一步(貪婪),
    // 避開界外 / 山 / 水 / 玩家格 / 其他怪。關閉時 = 原版「不移動、只相鄰反擊」。
    if (Configuration::getChaseMonsters()) {
        for (const auto &e : _enemies) {
            int dx = px - e->getX(), dy = py - e->getY();
            if (abs(dx) + abs(dy) <= 1) continue;          // 已相鄰/重疊 → 留給攻擊判定
            int sx = (dx > 0) - (dx < 0), sy = (dy > 0) - (dy < 0);
            bool xFirst = abs(dx) >= abs(dy);
            int cand[2][2] = {{xFirst ? sx : 0, xFirst ? 0 : sy},   // 先走距離大的軸
                              {xFirst ? 0 : sx, xFirst ? sy : 0}};  // 被擋再走另一軸
            for (auto &c : cand) {
                if (c[0] == 0 && c[1] == 0) continue;
                int nx = e->getX() + c[0], ny = e->getY() + c[1];
                if (nx < 0 || nx > BOUND_X_TILES || ny < 0 || ny > BOUND_Y_TILES) continue;
                auto t = _tiles[getTileOffset(nx, ny)]->getSpriteType();
                if (t == OverworldSpriteType::SpriteType::MOUNTAIN ||
                    t == OverworldSpriteType::SpriteType::WATER) continue;
                if (nx == px && ny == py) continue;        // 不踩玩家(留相鄰攻擊)
                bool occupied = false;
                for (const auto &o : _enemies)
                    if (o != e && o->getX() == nx && o->getY() == ny) { occupied = true; break; }
                if (occupied) continue;
                e->setCoordinates(nx, ny);
                break;
            }
        }
    }

    // 相鄰反擊(原版行為;追蹤模式下怪走到旁邊後也用這段攻擊)
    for (const auto &e : _enemies) {
        if (abs(e->getX() - px) + abs(e->getY() - py) == 1) {
            int dmg = Combat::reduceByArmor(2 + rand() % 5, *player);   // 經防具減免
            player->receiveDamage(dmg);
            CommandDisplay::writeLn(I18n::tf("ow.attacked", {e->getName(), to_string(dmg)}), false);
            if (player->isDead()) {
                respawnPlayer();
                break;
            }
        }
    }
}

// 死亡復活:回 Lord British 城堡(以世界起點代),HP 回滿、損失半數金幣。
void OverworldScreen::respawnPlayer() {
    auto p = _gameContext->getPlayer();
    CommandDisplay::writeLn(I18n::t("common.dead"), false);
    CommandDisplay::writeLn(I18n::t("respawn.revived"), false);
    p->setMoney(p->getMoney() / 2);
    p->setHP(p->getMaxHP());
    p->setOverworldX(20);
    p->setOverworldY(20);
    _gameContext->setScreen(ScreenType::Overworld);
}

void OverworldScreen::spawnNpcs() {
    // 測試 hook(env-gated,正常遊玩不啟用):在玩家右側強制生一隻怪以驗證相鄰反擊。
    if (getenv("U1_TEST_ADJ_SPAWN") && _enemies.empty()) {
        auto pl = _gameContext->getPlayer();
        int ax = pl->getOverworldX() + 1, ay = pl->getOverworldY();
        auto st = _spritesMap.find(OverworldSpriteType::SpriteType::WANDERING_WARLOCK)->second;
        auto tl = make_shared<OverworldTile>(ax, ay, st, make_shared<TileAnimation>());
        _enemies.push_back(make_shared<OverworldEnemy>(10, ax, ay, I18n::t("monster.wandering_warlock"), tl));
        return;
    }

    if ((int) _enemies.size() >= MOB_MAX) return;

    // 機率閘門:spawn_pct 倍率 + 基礎 ~87.5%/步(對齊 u2-cht / oracle)
    int spawnPct = Configuration::getSpawnPct();
    if (spawnPct < 100 && rand() % 100 >= spawnPct) return;
    if (rand() % 8 >= 7) return;

    auto player = _gameContext->getPlayer();
    int px = player->getOverworldX();
    int py = player->getOverworldY();

    // 視野內隨機可通行空格(離玩家至少 3 格,避免貼臉生成)
    constexpr int HALF_W = DISPLAY_SIZE_TILES_WIDTH / 2;
    constexpr int HALF_H = DISPLAY_SIZE_TILES_HEIGHT / 2;
    for (int attempt = 0; attempt < 12; attempt++) {
        int x = px + (rand() % (2 * HALF_W + 1)) - HALF_W;
        int y = py + (rand() % (2 * HALF_H + 1)) - HALF_H;
        if (abs(x - px) + abs(y - py) < 3) continue;        // 太近
        if (!isPassable(x, y)) continue;
        if (x == px && y == py) continue;
        bool occupied = false;
        for (const auto &e : _enemies) if (e->getX() == x && e->getY() == y) { occupied = true; break; }
        if (occupied) continue;

        auto spriteType = _spritesMap.find(OverworldSpriteType::SpriteType::WANDERING_WARLOCK)->second;
        auto tile = make_shared<OverworldTile>(x, y, spriteType, make_shared<TileAnimation>());
        _enemies.push_back(make_shared<OverworldEnemy>(10, x, y, I18n::t("monster.wandering_warlock"), tile));
        return;
    }
}

void OverworldScreen::setAttackMode(bool set) {
    if (set) {
        _attackMode = true;
        CommandDisplay::write(I18n::t("ow.dagger_attack"));
    } else {
        if (_attackMode) {
            _attackMode = false;
            CommandDisplay::writeLn(I18n::t("ow.no_target"), true);
        }
    }
}

string OverworldScreen::getCardinalPointFromDeltas(int deltaX, int deltaY) {
    if (deltaX < 0) {
        return I18n::t("dir.west");
    } else if (deltaX > 0) {
        return I18n::t("dir.east");
    } else if (deltaY > 0) {
        return I18n::t("dir.north");
    } else if (deltaY < 0) {
        return I18n::t("dir.south");
    }
}
