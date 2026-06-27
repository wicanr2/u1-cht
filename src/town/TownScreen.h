#pragma once

#include "../Screen.h"

#include <utility>
#include "Town.h"
#include "TownManager.h"
#include "../ItemCatalog.h"

class TownScreen : public Screen {
public:
    TownScreen(const shared_ptr<GameContext> &gameContext, SDL_Renderer *renderer);

    // 站在 counter 旁時依最近招牌字判定店家(authentic 定位);否則回 None。
    ShopType shopAtPlayer();

    void update(float elapsed) override;

    void draw(SDL_Renderer *renderer) override;

    void handle(const SDL_Event &e) override;

    void refresh();

private:
    static constexpr int PLAYER_INITIAL_TOWN_POSITION_X = 19;
    static constexpr int PLAYER_INITIAL_TOWN_POSITION_Y = 17;
    static constexpr int PLAYER_INITIAL_CASTLE_POSITION_X = 0;
    static constexpr int PLAYER_INITIAL_CASTLE_POSITION_Y = 7;

    shared_ptr<TownManager> _townManager;
    shared_ptr<TownSpriteTypeLoader> _townSpriteTypeLoader;

    shared_ptr<Town> _currentTown = nullptr;
    shared_ptr<TownTile> _playerTile;

    void resetPlayerPosition();

    void playerMove(CardinalPoint direction);
};