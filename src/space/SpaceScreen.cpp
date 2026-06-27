#include "SpaceScreen.h"
#include "../common/I18n.h"
#include "../CommandDisplay.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

// 320x200 世界 target 內的星海座標範圍
static constexpr float MINX = 10, MAXX = 310, TOPY = 12, SHIPY = 170, BOTY = 184;

SpaceScreen::SpaceScreen(shared_ptr<GameContext> gameContext) : Screen(std::move(gameContext)) {}

void SpaceScreen::initStars() {
    _stars.clear();
    for (int i = 0; i < 60; i++)
        _stars.push_back({(float)(10 + rand() % 300), (float)(10 + rand() % 180)});
}

void SpaceScreen::reset() {
    _shipX = 152.0f;
    _bullets.clear();
    _enemies.clear();
    _kills = 0;
    _spawnTimer = 0;
    _fireCooldown = 0;
    initStars();
    _initialized = true;
    CommandDisplay::writeLn(I18n::t("space.enter"), false);
}

void SpaceScreen::fire() {
    if (_fireCooldown > 0) return;
    _bullets.push_back({_shipX, SHIPY - 6});
    _fireCooldown = 8.0f;   // frames
}

void SpaceScreen::handle(const SDL_Event &e) {
    if (e.type != SDL_KEYDOWN) return;
    auto k = e.key.keysym.sym;
    if (k == SDLK_LEFT)  _shipX -= 12;
    else if (k == SDLK_RIGHT) _shipX += 12;
    else if (k == SDLK_SPACE || k == SDLK_a) fire();
    else if (k == SDLK_ESCAPE) _gameContext->setScreen(ScreenType::Overworld);  // 撤離
    if (_shipX < MINX) _shipX = MINX;
    if (_shipX > MAXX) _shipX = MAXX;
}

void SpaceScreen::update(float) {
    if (!_initialized) reset();
    if (_fireCooldown > 0) _fireCooldown -= 1;

    // 子彈上行
    for (auto &b : _bullets) b.y -= 5;
    _bullets.erase(std::remove_if(_bullets.begin(), _bullets.end(),
                                  [](const P &b) { return b.y < TOPY - 4; }), _bullets.end());

    // 生敵艦(自上方)
    _spawnTimer -= 1;
    if (_spawnTimer <= 0 && (int)_enemies.size() < 6) {
        _enemies.push_back({(float)(20 + rand() % 280), TOPY});
        _spawnTimer = 45 + rand() % 40;
    }
    // 敵艦下行 + 輕微橫移
    auto p = _gameContext->getPlayer();
    for (auto &en : _enemies) {
        en.y += 1.1f;
        en.x += ((en.y > 0) ? ((((int)en.y / 17) % 2) ? 0.5f : -0.5f) : 0);
    }
    // 敵艦觸底 → 玩家受損
    for (auto it = _enemies.begin(); it != _enemies.end();) {
        if (it->y >= BOTY) {
            p->receiveDamage(5);
            it = _enemies.erase(it);
            if (p->isDead()) {  // 護盾耗盡 → 撤回世界(回滿,不致全失)
                p->setHP(p->getMaxHP());
                CommandDisplay::writeLn(I18n::t("space.shields_gone"), false);
                _gameContext->setScreen(ScreenType::Overworld);
                return;
            }
        } else ++it;
    }
    // 子彈 × 敵艦 命中判定
    for (auto bi = _bullets.begin(); bi != _bullets.end();) {
        bool hit = false;
        for (auto ei = _enemies.begin(); ei != _enemies.end(); ++ei) {
            if (std::fabs(bi->x - ei->x) < 7 && std::fabs(bi->y - ei->y) < 7) {
                _enemies.erase(ei);
                hit = true;
                _kills++;
                if (getenv("U1_SPACE_DEBUG")) fprintf(stderr, "SPACEKILL kills=%d\n", _kills);
                if (_kills >= KILLS_TO_ACE) {
                    p->setSpaceAce(true);
                    CommandDisplay::writeLn(I18n::t("space.ace"), false);
                    _gameContext->setScreen(ScreenType::Overworld);
                    return;
                }
                break;
            }
        }
        if (hit) bi = _bullets.erase(bi); else ++bi;
    }
}

void SpaceScreen::draw(SDL_Renderer *renderer) {
    if (!_initialized) reset();
    // 黑底星海
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_Rect bg = {0, 0, 320, 200};
    SDL_RenderFillRect(renderer, &bg);

    // 星點
    SDL_SetRenderDrawColor(renderer, 200, 200, 220, 255);
    for (const auto &s : _stars) {
        SDL_Rect r = {(int)s.x, (int)s.y, 1, 1};
        SDL_RenderFillRect(renderer, &r);
    }

    // 玩家船(青色三角)
    SDL_SetRenderDrawColor(renderer, 0x42, 0xFF, 0xFF, 255);
    int sx = (int)_shipX;
    SDL_RenderDrawLine(renderer, sx, (int)SHIPY - 6, sx - 5, (int)SHIPY + 4);
    SDL_RenderDrawLine(renderer, sx, (int)SHIPY - 6, sx + 5, (int)SHIPY + 4);
    SDL_RenderDrawLine(renderer, sx - 5, (int)SHIPY + 4, sx + 5, (int)SHIPY + 4);

    // 子彈(黃)
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0x60, 255);
    for (const auto &b : _bullets) {
        SDL_Rect r = {(int)b.x, (int)b.y, 1, 4};
        SDL_RenderFillRect(renderer, &r);
    }

    // 敵艦(紅色菱形/方塊)
    SDL_SetRenderDrawColor(renderer, 0xFF, 0x50, 0x50, 255);
    for (const auto &en : _enemies) {
        int ex = (int)en.x, ey = (int)en.y;
        SDL_RenderDrawLine(renderer, ex - 5, ey, ex, ey - 5);
        SDL_RenderDrawLine(renderer, ex, ey - 5, ex + 5, ey);
        SDL_RenderDrawLine(renderer, ex + 5, ey, ex, ey + 5);
        SDL_RenderDrawLine(renderer, ex, ey + 5, ex - 5, ey);
    }

    // 擊落進度 pips(頂部一排,達標 KILLS_TO_ACE)
    for (int i = 0; i < KILLS_TO_ACE; i++) {
        if (i < _kills) SDL_SetRenderDrawColor(renderer, 0x60, 0xFF, 0x60, 255);
        else SDL_SetRenderDrawColor(renderer, 0x40, 0x40, 0x40, 255);
        SDL_Rect r = {12 + i * 8, 4, 5, 4};
        SDL_RenderFillRect(renderer, &r);
    }
}
