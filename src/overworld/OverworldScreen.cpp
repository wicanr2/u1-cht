#include "OverworldScreen.h"
#include "TileTypeLoader.h"
#include "../common/graphics/PixelDecodeStrategy.h"
#include "Constants.h"
#include "TileAnimation.h"
#include "../common/Fonts.h"
#include "../CommandDisplay.h"
#include <iostream>
#include <stdexcept>
#include "../common/ShapeUtils.h"
#include "../Configuration.h"

void OverworldScreen::init(SDL_Renderer *renderer,
                           PixelDecodeStrategy *pixelDecodeStrategy, const string &tilesFsPath) {
    _tiles.clear();
    _spritesMap.clear();

    // load all sprite types.
    auto spriteLoader = make_unique<TileTypeLoader>();
    auto spriteTypes = spriteLoader->loadOverworldSprites(tilesFsPath, pixelDecodeStrategy, renderer);

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
        CommandDisplay::writeLn("山脈無法通行!", true);
        return;
    }

    if (typeTypeSteppedOn == OverworldSpriteType::SpriteType::WATER) {
        CommandDisplay::writeLn("無法在水上行走!", true);
        return;
    }

    // Don't allow player to walk past an enemy
    for (const auto &enemy: _enemies) {
        if (enemy->getX() == playerX && enemy->getY() == playerY) {
            CommandDisplay::writeLn("被" + enemy->getName() + "擋住!", true);
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

    //target->receiveDamage(1);
    CommandDisplay::writeLn(getCardinalPointFromDeltas(deltaX, deltaY), true);

    CommandDisplay::writeLn("擊殺了" + target->getName() + "!", false);
    _enemies.erase(_enemies.begin() + index, _enemies.begin() + index + 1);

    _attackMode = false;
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

    // 時間累加器:滿 100 走一個時間 tick(目前 open_ultima 尚無怪物移動/食物消耗,
    // 先建立 turn-based 節奏,後續 step_mobs / 食物 掛此處)。
    _timeAccum += Configuration::getSpeedPct();
    while (_timeAccum >= 100) {
        _timeAccum -= 100;
        // TODO: step_mobs(); 食物 -= 1; 等隨時間 tick 的回合制邏輯
    }
}

void OverworldScreen::spawnNpcs() {
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
        _enemies.push_back(make_shared<OverworldEnemy>(10, x, y, "遊蕩術士", tile));
        return;
    }
}

void OverworldScreen::setAttackMode(bool set) {
    if (set) {
        _attackMode = true;
        CommandDisplay::write("用匕首攻擊:");
    } else {
        if (_attackMode) {
            _attackMode = false;
            CommandDisplay::writeLn("沒有目標!", true);
        }
    }
}

string OverworldScreen::getCardinalPointFromDeltas(int deltaX, int deltaY) {
    if (deltaX < 0) {
        return "西";
    } else if (deltaX > 0) {
        return "東";
    } else if (deltaY > 0) {
        return "北";
    } else if (deltaY < 0) {
        return "南";
    }
}
