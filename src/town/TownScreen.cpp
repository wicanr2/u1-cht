#include "TownScreen.h"
#include "../common/I18n.h"
#include "../common/graphics/EGARowPlanarDecodeStrategy.h"
#include "../CommandDisplay.h"
#include "../Configuration.h"

TownScreen::TownScreen(const shared_ptr<GameContext> &gameContext, SDL_Renderer *renderer)
        : Screen(gameContext) {
    _townSpriteTypeLoader = make_shared<TownSpriteTypeLoader>();
    _townSpriteTypeLoader->init(Configuration::getEgaTownFilePath(),
                                make_unique<EGARowPlanarDecodeStrategy>(8, 8).get(),
                                renderer);
    _townManager = make_shared<TownManager>();
    _townManager->init(_townSpriteTypeLoader, Configuration::getTownMapFilePath());

    auto playerSpriteType = _townSpriteTypeLoader->getSpriteType(TownSpriteType::SpriteType::PERSON_PLAYER);
    _playerTile = make_shared<TownTile>(PLAYER_INITIAL_TOWN_POSITION_X, PLAYER_INITIAL_TOWN_POSITION_Y,
                                        playerSpriteType);
}

void TownScreen::update(float elapsed) {

}

void TownScreen::handle(const SDL_Event &e) {
    if (e.type == SDL_KEYDOWN) {
        auto pressedKey = e.key.keysym.sym;

        switch (pressedKey) {
            case SDLK_e:
                _gameContext->setScreen(ScreenType::Overworld);
                break;
            case SDLK_UP:
                playerMove(CardinalPoint::North);
                break;
            case SDLK_DOWN:
                playerMove(CardinalPoint::South);
                break;
            case SDLK_LEFT:
                playerMove(CardinalPoint::West);
                break;
            case SDLK_RIGHT:
                playerMove(CardinalPoint::East);
                break;
            default:
                break;
        }
    }
}

void TownScreen::refresh() {
    int x = _gameContext->getPlayer()->getOverworldX();
    int y = _gameContext->getPlayer()->getOverworldY();

    if (_gameContext->isInCastle()) {
        _currentTown = _townManager->getCastle(x, y);
    } else {
        _currentTown = _townManager->getTown(x, y);
    }

    // reset player starting position on town.
    resetPlayerPosition();

    if (getenv("U1_DUMP_TOWN")) {   // 一次性:印城鎮格局以了解招牌/店員排布
        using S = TownSpriteType::SpriteType;
        for (int ty = 0; ty < Town::HEIGHT; ty++) {
            string row;
            for (int tx = 0; tx < Town::WIDTH; tx++) {
                auto t = _currentTown->getTile(tx, ty);
                S s = t ? t->getType() : S::NONE;
                char c = '.';
                if (s >= S::A && s <= S::Z) c = 'A' + ((int)s - (int)S::A);
                else if (s == S::PERSON_MERCHANT) c = '$';
                else if (s == S::PERSON_KING) c = 'K';
                else if (s == S::PERSON_GUARD) c = 'g';
                else if (s == S::PERSON_PRISONER) c = '!';
                else if (s == S::PERSON_JESTER_1 || s == S::PERSON_JESTER_2) c = 'j';
                else if (s >= S::COUNTER_LEFT_RIGHT && s <= S::COUNTER_RIGHT) c = '=';
                else if (s == S::SOLID_WALL || s == S::BRICK_WALL) c = '#';
                else if (s == S::WATER) c = '~';
                row += c;
            }
            printf("TOWNDUMP %02d %s\n", ty, row.c_str());
        }
        // 自測:對每個 counter,把玩家放到其左側格,印 shopAtPlayer() 判定結果
        int sx = _playerTile->getX(), sy = _playerTile->getY();
        for (int ty = 0; ty < Town::HEIGHT; ty++)
            for (int tx = 0; tx < Town::WIDTH; tx++) {
                auto t = _currentTown->getTile(tx, ty);
                if (!t) continue;
                S s = t->getType();
                if (s >= S::COUNTER_LEFT_RIGHT && s <= S::COUNTER_RIGHT && tx > 0) {
                    _playerTile->setCoordinates(tx - 1, ty);
                    int st = (int)shopAtPlayer();
                    if (st != (int)ShopType::None)
                        printf("SHOPDET counter@%d,%d -> shopType=%d\n", tx, ty, st);
                }
            }
        _playerTile->setCoordinates(sx, sy);
        fflush(stdout);
    }
}

// 招牌字串 → 店家類型(U1 城鎮招牌:WEAPONS/ARMS、ARMOUR、MAGIC、GROCERY/FOOD、INN/PUB、GUILD/STABLE…)
static ShopType shopTypeForWord(const std::string &w) {
    auto has = [&](const char *s) { return w.find(s) != std::string::npos; };
    if (has("ARMOUR") || has("ARMOR")) return ShopType::Armor;
    if (has("WEAPON") || has("ARMS")) return ShopType::Weapon;
    if (has("MAGIC")) return ShopType::Magic;
    if (has("GROCER") || has("FOOD")) return ShopType::Food;
    if (has("INN") || has("PUB") || has("TAVERN") || has("ALE")) return ShopType::Pub;
    if (has("GUILD") || has("STABLE") || has("HORSE") || has("DOCK") || has("SHIP")) return ShopType::Transport;
    return ShopType::None;
}

ShopType TownScreen::shopAtPlayer() {
    if (!_currentTown) return ShopType::None;
    using S = TownSpriteType::SpriteType;
    int px = _playerTile->getX(), py = _playerTile->getY();

    auto typeAt = [&](int x, int y) -> S {
        if (x < 0 || y < 0 || x >= Town::WIDTH || y >= Town::HEIGHT) return S::NONE;
        auto t = _currentTown->getTile(x, y);
        return t ? t->getType() : S::NONE;
    };
    auto isCounter = [&](S s) { return s >= S::COUNTER_LEFT_RIGHT && s <= S::COUNTER_RIGHT; };

    // 必須站在 counter 旁(含對角),否則不算在店裡
    bool nearCounter = false;
    for (int dy = -1; dy <= 1 && !nearCounter; dy++)
        for (int dx = -1; dx <= 1; dx++)
            if (isCounter(typeAt(px + dx, py + dy))) { nearCounter = true; break; }
    if (!nearCounter) return ShopType::None;

    // 掃描全城招牌詞(水平字母連續段),取離玩家最近、可辨識的店家
    ShopType best = ShopType::None;
    int bestDist = 1 << 30;
    for (int y = 0; y < Town::HEIGHT; y++) {
        std::string word;
        int startX = -1;
        for (int x = 0; x <= Town::WIDTH; x++) {
            S s = (x < Town::WIDTH) ? typeAt(x, y) : S::NONE;
            bool letter = (s >= S::A && s <= S::Z);
            if (letter) {
                if (word.empty()) startX = x;
                word += (char)('A' + ((int)s - (int)S::A));
            } else {
                if (word.size() >= 3) {
                    ShopType t = shopTypeForWord(word);
                    if (t != ShopType::None) {
                        int cx = (startX + x - 1) / 2;
                        int d = abs(cx - px) + abs(y - py);
                        if (d < bestDist) { bestDist = d; best = t; }
                    }
                }
                word.clear();
            }
        }
    }
    return best;
}

void TownScreen::draw(SDL_Renderer *renderer) {
    SDL_Rect defaultViewport = {MAIN_VIEWPORT_PADDING, MAIN_VIEWPORT_PADDING, WIDTH, HEIGHT};
    SDL_RenderSetViewport(renderer, &defaultViewport);

    clearScreen(renderer);

    auto tiles = _currentTown->getTiles();
    constexpr SDL_Rect camera = {0, 0, Screen::WIDTH, Screen::HEIGHT};
    for (const auto &tile: tiles) {
        tile->draw(renderer, camera);
    }

    _playerTile->draw(renderer, camera);
}

void TownScreen::resetPlayerPosition() {
    if (_currentTown->isCastle()) {
        _playerTile->setCoordinates(PLAYER_INITIAL_CASTLE_POSITION_X,
                                    PLAYER_INITIAL_CASTLE_POSITION_Y);
    } else {
        _playerTile->setCoordinates(PLAYER_INITIAL_TOWN_POSITION_X,
                                    PLAYER_INITIAL_TOWN_POSITION_Y);
    }
}

void TownScreen::playerMove(CardinalPoint direction) {
    int nextX = _playerTile->getX();
    int nextY = _playerTile->getY();

    switch (direction) {
        case CardinalPoint::North:
            nextY--;
            break;
        case CardinalPoint::South:
            nextY++;
            break;
        case CardinalPoint::East:
            nextX++;
            break;
        case CardinalPoint::West:
            nextX--;
            break;
    }

    bool blocked = nextX < 0 ||
                   nextX >= Town::WIDTH ||
                   nextY < 0 ||
                   nextY >= Town::HEIGHT;

    if (!blocked) {
        auto nextTile = _currentTown->getTile(nextX, nextY);
        TownSpriteType::SpriteType spriteType = nextTile->getType();
        blocked = spriteType != TownSpriteType::SpriteType::FLOOR && spriteType != TownSpriteType::SpriteType::NONE;
    }

    if (blocked) {
        CommandDisplay::writeLn(I18n::t("town.blocked"), true);
        return;
    }

    _playerTile->setCoordinates(nextX, nextY);
}
