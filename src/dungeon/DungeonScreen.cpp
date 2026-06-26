#include "DungeonScreen.h"
#include "../CommandDisplay.h"
#include "../Configuration.h"
#include "Door.h"
#include "Ladder.h"
#include "../common/ShapeUtils.h"
#include "../common/Fonts.h"
#include "../common/Colors.h"
#include "enemies/Thief.h"
#include <iostream>
#include <algorithm>
#include <cstdlib>

using namespace std;

void DungeonScreen::update(float elapsed) {
    if (!_drawEnabled) {
        _drawCounter += elapsed;
        if (_drawCounter >= 100) {
            _drawEnabled = true;
            _drawCounter = 0;
        }
    }
}

void DungeonScreen::draw(SDL_Renderer *renderer) {
    ShapeUtils::drawRoundedCorners(renderer);
    ShapeUtils::drawDungeonBorders(renderer);

    _orientationLabel->loadFromRenderedText(Fonts::cjk(), renderer, CardinalPointUtils::toString(
            _gameContext->getPlayer()->getDungeonOrientation()), Colors::TEXT_COLOR);
    _orientationLabel->render(renderer, 144, 150);

    _levelLabel->loadFromRenderedText(Fonts::cjk(), renderer,
                                      "第 " + to_string(_gameContext->getPlayer()->getDungeonLevel() + 1) + " 層",
                                      Colors::TEXT_COLOR);
    _levelLabel->render(renderer, 272 / 2, -2);

    // TODO: not too happy with the viewport thingy
    SDL_Rect defaultViewport = {MAIN_VIEWPORT_PADDING, MAIN_VIEWPORT_PADDING, WIDTH, HEIGHT};
    SDL_RenderSetViewport(renderer, &defaultViewport);

    clearScreen(renderer);

    // the _drawEnabled flag controls drawing to the screen.
    // this is to have a flicker effect similar to the original game so that the player sees a small delay in the drawing after an action.
    if (!_drawEnabled) return;

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);

    constexpr int DRAWING_PADDING = 8;

    if (_vision[0].feature == DungeonFeature::Door) {
        // We're standing inside a doorway, this screws up everything.
        // If looking through the doorway, we only care about the following two spaces: the next corridor space, and the wall after it.
        // Remember this whole dungeon design depends on everything being laid down in straight corridors, it's a mess!
        if (_vision.size() == 3) {
            // We're looking through the doorway.
            // Draw the arch and what's after it.

            // Draw arch first
            // Top
            SDL_RenderDrawLine(renderer, 16, 8, 16 + 272, 8);
            // Bottom left
            SDL_RenderDrawLine(renderer, 16, 135, 16 + 65, 135);
            SDL_RenderDrawLine(renderer, 80, 135, 80, 135 - 99);

            SDL_RenderDrawLine(renderer, 80, 36, 80 + 144, 36);

            SDL_RenderDrawLine(renderer, 80 + 144, 36, 80 + 144, 36 + 99);
            SDL_RenderDrawLine(renderer, 80 + 144, 36 + 99, 80 + 144 + 65, 36 + 99);

            // Draw what's after the arch, it should only be either a straight wall, a hidden door (looks like a wall) or another door.
            SDL_RenderDrawLine(renderer, 16 + 65, 54, 16 + 65 + 143, 54);
            SDL_RenderDrawLine(renderer, 16 + 65, 89, 16 + 65 + 143, 89);

            // If there is a door at the other side, draw it
            if (_vision[2].feature == DungeonFeature::Door) {
                SDL_RenderDrawLine(renderer, 134, 89, 134, 89 - 26);
                SDL_RenderDrawLine(renderer, 134, 89 - 26, 134 + 35, 89 - 26);
                SDL_RenderDrawLine(renderer, 134 + 35, 89 - 26, 134 + 35, 89 - 26 + 26);
            }
        } else {
            // We're inside a doorway, but facing the side walls, nothing to see but a wall
            SDL_RenderDrawLine(renderer, 16, 8, 16 + 272, 8);
            SDL_RenderDrawLine(renderer, 16, 135, 80 + 144 + 65, 135);
        }
    } else {
        // First draw all the walls
        if (_vision.size() == 2) {
            // 2 tiles of vision means the tile we are in right now, and the next tile is a wall/door (if it wasn't a wall, we'd be able to see past it)
            // x1 --> left point, x2 --> right point, y1 --> upper point, y2 --> lower point
            constexpr int oppositeWallX1 = 80;
            constexpr int oppositeWallY1 = 36;
            constexpr int oppositeWallX2 = oppositeWallX1 + 144 - 1;
            constexpr int oppositeWallY2 = oppositeWallY1 + 72 - 1;

            // draw the upper and lower lines of the opposite wall.
            SDL_RenderDrawLine(renderer, oppositeWallX1, oppositeWallY1, oppositeWallX2, oppositeWallY1);
            SDL_RenderDrawLine(renderer, oppositeWallX1, oppositeWallY2, oppositeWallX2, oppositeWallY2);

            if (isWalledFeature(_vision[0].left)) {
                SDL_RenderDrawLine(renderer, UPPER_LEFT_X, UPPER_LEFT_Y, oppositeWallX1, oppositeWallY1);
                SDL_RenderDrawLine(renderer, oppositeWallX1, oppositeWallY1, oppositeWallX1, oppositeWallY2);
                SDL_RenderDrawLine(renderer, LOWER_LEFT_X, LOWER_LEFT_Y, oppositeWallX1, oppositeWallY2);
            } else {
                SDL_RenderDrawLine(renderer, oppositeWallX1, oppositeWallY1, DRAWING_PADDING, oppositeWallY1);
                SDL_RenderDrawLine(renderer, oppositeWallX1, oppositeWallY2, DRAWING_PADDING, oppositeWallY2);
            }

            if (isWalledFeature(_vision[0].right)) {
                SDL_RenderDrawLine(renderer, UPPER_RIGHT_X, UPPER_RIGHT_Y, oppositeWallX2, oppositeWallY1);
                SDL_RenderDrawLine(renderer, oppositeWallX2, oppositeWallY1, oppositeWallX2, oppositeWallY2);
                SDL_RenderDrawLine(renderer, LOWER_RIGHT_X, LOWER_RIGHT_Y, oppositeWallX2, oppositeWallY2);
            } else {
                SDL_RenderDrawLine(renderer, oppositeWallX2, oppositeWallY1, WIDTH - DRAWING_PADDING, oppositeWallY1);
                SDL_RenderDrawLine(renderer, oppositeWallX2, oppositeWallY2, WIDTH - DRAWING_PADDING, oppositeWallY2);
            }
        } else {

            drawLeftWalls(renderer);
            drawRightWalls(renderer);

            // This is the vision "horizon"
            SDL_RenderDrawLine(renderer, 149, 72, 155, 72);
            SDL_RenderDrawLine(renderer, 149, 73, 155, 73);

            // If there is something at 5 spots away, it could either mean it's a wall, or is just the limit of visibility.
            if (_vision.size() == 5 && isWalledFeature(_vision[4].feature)) {
                SDL_Rect oppositeWallRect = {143, 69, 18, 7};

                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
                SDL_RenderFillRect(renderer, &oppositeWallRect);

                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
                SDL_RenderDrawRect(renderer, &oppositeWallRect);
            } else if (_vision.size() == 4) {
                SDL_Rect oppositeWallRect = {134, 64, 36, 17};

                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
                SDL_RenderFillRect(renderer, &oppositeWallRect);

                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
                SDL_RenderDrawRect(renderer, &oppositeWallRect);
            } else if (_vision.size() == 3) {
                SDL_Rect oppositeWallRect = {116, 55, 72, 35};

                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
                SDL_RenderFillRect(renderer, &oppositeWallRect);

                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
                SDL_RenderDrawRect(renderer, &oppositeWallRect);
            }
        }
    }

    // draw sprites in reverse order so that the closer sprites are drawn on top of the farther ones
    int distance = _vision.size() - 1;
    while (distance >= 0) {
        auto tile = _vision[distance];

        if (tile.right == DungeonFeature::Door) {
            Door::draw(renderer, distance, Direction::Right);
        }

        if (tile.left == DungeonFeature::Door) {
            Door::draw(renderer, distance, Direction::Left);
        }

        if (tile.feature == DungeonFeature::Door) {
            if (distance != 0) {
                Door::drawFrontFacing(renderer, distance);
            }
        }

        if (tile.enemy != nullptr && !tile.enemy->isDead()) {
            tile.enemy->draw(renderer, distance);
        }

        if (tile.ladder != nullptr) {
            auto currentOrientation = _gameContext->getPlayer()->getDungeonOrientation();
            if (currentOrientation == CardinalPoint::East || currentOrientation == CardinalPoint::West) {
                Ladder::drawFromSide(renderer, distance, tile.ladder->goingUp);
            } else {
                Ladder::drawFront(renderer, distance, tile.ladder->goingUp);
            }
        }

        distance--;
    }
}

void DungeonScreen::handle(const SDL_Event &e) {
    auto player = _gameContext->getPlayer();

    if (e.type == SDL_KEYDOWN) {
        auto pressedKey = e.key.keysym.sym;

        switch (pressedKey) {
            case SDLK_UP:
                moveForward();
                break;
            case SDLK_DOWN:
                player->dungeonTurn(Direction::Right);
                player->dungeonTurn(Direction::Right);
                CommandDisplay::writeLn("向後轉", true);
                break;
            case SDLK_LEFT:
                player->dungeonTurn(Direction::Left);
                CommandDisplay::writeLn("向左轉", true);
                break;
            case SDLK_RIGHT:
                player->dungeonTurn(Direction::Right);
                CommandDisplay::writeLn("向右轉", true);
                break;
            case SDLK_a:
                doCombatRound(true);
                break;
            case SDLK_k:
                climbLadder();
                break;
            default:
                return;
        }

        // 玩家走一步 = 一回合 → 地牢怪行動(逐玩家移動 + 相鄰攻擊)
        if (!player->isDead()) {
            dungeonMonsterTurn();
        }

        _vision = _dungeon->getVisible(player->getDungeonLevel(), player->getDungeonX(), player->getDungeonY(),
                                       player->getDungeonOrientation());

        for (const auto &visibleTile: _vision) {
            if (visibleTile.enemy) {
                CommandDisplay::writeLn(visibleTile.enemy->getName(), false);
                // Only display the name of the first enemy in the conga line.
                break;
            }
        }

        _drawEnabled = false;
    }
}

void DungeonScreen::moveForward() {
    auto player = _gameContext->getPlayer();
    if (_vision[1].feature == DungeonFeature::Wall || _vision[1].enemy != nullptr) {
        CommandDisplay::writeLn("前進—前方受阻!", true);
        return;
    }

    // If we arrive to this point, we're either inside a door, or in a hallway and can move forward.
    player->dungeonMoveForward();
    CommandDisplay::writeLn("前進", true);

    cout << "Moved to: " << player->getDungeonX() << ", " << player->getDungeonY() << "\n";
}

void DungeonScreen::drawLeftWalls(SDL_Renderer *renderer) {
    SDL_RenderDrawLine(renderer, 8, 0, 149, 72);
    SDL_RenderDrawLine(renderer, 8, 144, 149, 73);
}

void DungeonScreen::drawRightWalls(SDL_Renderer *renderer) {
    SDL_RenderDrawLine(renderer, 296, 0, 155, 72);
    SDL_RenderDrawLine(renderer, 296, 144, 155, 73);
}

void DungeonScreen::doCombatRound(bool playerAttacks) {
    // 玩家攻擊;怪物的移動與反擊由 dungeonMonsterTurn() 在回合結束統一處理
    if (playerAttacks) doPlayerAttack();
}

// 地牢一回合:怪先逐玩家移動,再對相鄰玩家攻擊(忠於原版 U1)
void DungeonScreen::dungeonMonsterTurn() {
    auto player = _gameContext->getPlayer();
    moveEnemiesToward(player->getDungeonX(), player->getDungeonY());
    doMonsterAttacks();

    // 時間 tick:食物消耗 / 飢餓(與地面一致,參考 u2-cht)
    _timeAccum += Configuration::getSpeedPct();
    while (_timeAccum >= 100) {
        _timeAccum -= 100;
        player->consumeFood(1);
        if (player->isStarving()) {
            player->receiveDamage(2);
            CommandDisplay::writeLn("飢餓!損失 2 點生命", false);
            if (player->isDead()) { CommandDisplay::writeLn("你已餓死!", false); break; }
        }
    }
}

// 目標格可否讓怪進入:可走且未被玩家或其他怪佔據
bool DungeonScreen::dungeonCellFree(int x, int y, const shared_ptr<Enemy> &self) {
    auto player = _gameContext->getPlayer();
    int level = player->getDungeonLevel();
    if (!_dungeon->isMonsterWalkable(level, x, y)) return false;
    if (x == player->getDungeonX() && y == player->getDungeonY()) return false;
    for (const auto &e : _dungeon->getLevelEnemies(level)) {
        if (e != self && !e->isDead() && e->getX() == x && e->getY() == y) return false;
    }
    return true;
}

// beeline 貪婪逐玩家:相鄰(含對角)不動(交給攻擊),否則朝玩家走一格,
// 取 X/Y 距離較大軸優先;被擋則退而求其次走另一軸。
void DungeonScreen::moveEnemiesToward(int px, int py) {
    int level = _gameContext->getPlayer()->getDungeonLevel();
    for (const auto &e : _dungeon->getLevelEnemies(level)) {
        if (e->isDead()) continue;
        int dx = px - e->getX();
        int dy = py - e->getY();
        if (std::max(std::abs(dx), std::abs(dy)) <= 1) continue;  // 已相鄰 → 不移動,等攻擊

        int sx = (dx > 0) - (dx < 0);   // -1/0/+1
        int sy = (dy > 0) - (dy < 0);

        // 候選步序:距離大的軸優先,失敗再試另一軸
        int cand[2][2];
        if (std::abs(dx) >= std::abs(dy)) { cand[0][0]=sx; cand[0][1]=0; cand[1][0]=0; cand[1][1]=sy; }
        else                              { cand[0][0]=0; cand[0][1]=sy; cand[1][0]=sx; cand[1][1]=0; }

        for (auto &c : cand) {
            if (c[0] == 0 && c[1] == 0) continue;
            int nx = e->getX() + c[0], ny = e->getY() + c[1];
            if (dungeonCellFree(nx, ny, e)) { e->setPosition(nx, ny); break; }
        }
    }
}

void DungeonScreen::doPlayerAttack() {
    // attack monster in front of player
    auto player = _gameContext->getPlayer();
    shared_ptr<Enemy> enemy = nullptr;

    for (const auto &tile: _vision) {
        if (tile.enemy != nullptr) {
            enemy = tile.enemy;
            break;
        }
    }

    if (enemy != nullptr) {
        CommandDisplay::writeLn("徒手攻擊", true);

        enemy->receiveDamage(1);
        if (enemy->isDead()) {
            CommandDisplay::writeLn("命中!擊殺" + enemy->getName() + "!", false);
        } else {
            CommandDisplay::writeLn("命中!造成 1 點傷害", false);
        }
    }
}

void DungeonScreen::doMonsterAttacks() {
    auto player = _gameContext->getPlayer();

    for (const auto &enemy: _dungeon->getLevelEnemies(player->getDungeonLevel())) {
        if (enemy->isDead()) continue;
        int dx = std::abs(enemy->getX() - player->getDungeonX());
        int dy = std::abs(enemy->getY() - player->getDungeonY());
        // 相鄰即攻擊(含對角;原版 U1 怪可對角攻擊,玩家不行)
        if (std::max(dx, dy) == 1) {
            doMonsterAttack(enemy);
            if (player->isDead()) break;
        }
    }
}

void DungeonScreen::doMonsterAttack(const shared_ptr<Enemy> &enemy) {
    auto player = _gameContext->getPlayer();

    CommandDisplay::writeLn("遭" + enemy->getName() + "攻擊!", false);
    CommandDisplay::writeLn("命中!造成 " + to_string(enemy->getDamage()) + " 點傷害!", false);

    player->receiveDamage(enemy->getDamage());

    if (player->isDead()) {
        CommandDisplay::writeLn("你已死亡!", false);
    }
}

void DungeonScreen::climbLadder() {
    if (_vision[0].ladder == nullptr) {
        CommandDisplay::writeLn("攀爬什麼?", true);
        return;
    }

    auto player = _gameContext->getPlayer();
    auto currentLevel = player->getDungeonLevel();
    auto ladder = _vision[0].ladder;
    if (ladder->goingUp) {
        if (currentLevel == 0) {
            _gameContext->setScreen(ScreenType::Overworld);
            CommandDisplay::writeLn("向上攀爬至第 " + to_string(currentLevel) + " 層", true);
        } else {
            player->setDungeonLevel(--currentLevel);
            CommandDisplay::writeLn("向上攀爬至第 " + to_string(currentLevel + 1) + " 層", true);
        }
    } else {
        player->setDungeonLevel(++currentLevel);
        CommandDisplay::writeLn("向下攀爬至第 " + to_string(currentLevel + 1) + " 層", true);
    }

    player->setDungeonX(ladder->toX);
    player->setDungeonY(ladder->toY);
}
