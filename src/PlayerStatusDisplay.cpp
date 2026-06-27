#include "PlayerStatusDisplay.h"
#include "common/I18n.h"

#include <utility>
#include "common/Fonts.h"
#include "common/Colors.h"

PlayerStatusDisplay::PlayerStatusDisplay(SDL_Renderer *renderer, shared_ptr<Player> player) {
    _textureLine1 = make_unique<LTexture>();
    _textureLine2 = make_unique<LTexture>();
    _textureXP = make_unique<LTexture>();
    _textureMoney = make_unique<LTexture>();
    _background = make_unique<LTexture>();
    _background->loadFromColor(renderer, WIDTH, HEIGHT, 0, 0, 0, 0);

    _player = std::move(player);
}

void PlayerStatusDisplay::draw(SDL_Renderer *renderer) {
    // 640 高解析覆蓋層:viewport 與座標 x2,字型 16px
    constexpr int S = 2;
    SDL_Rect viewport = {POSITION_X * S, POSITION_Y * S, WIDTH * S, HEIGHT * S};
    SDL_RenderSetViewport(renderer, &viewport);

    // 黑底面板
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
    SDL_Rect bg = {0, 0, WIDTH * S, HEIGHT * S};
    SDL_RenderFillRect(renderer, &bg);

    _textureLine1->loadFromRenderedText(Fonts::cjkUi(), renderer, I18n::t("status.hp") + to_string(_player->getHP()),
                                        Colors::TEXT_COLOR);
    _textureLine2->loadFromRenderedText(Fonts::cjkUi(), renderer, I18n::t("status.food") + to_string(_player->getFood()),
                                        Colors::TEXT_COLOR);
    _textureXP->loadFromRenderedText(Fonts::cjkUi(), renderer, I18n::t("status.xp") + to_string(_player->getXP()),
                                     Colors::TEXT_COLOR);
    _textureMoney->loadFromRenderedText(Fonts::cjkUi(), renderer, I18n::t("status.money") + to_string(_player->getMoney()),
                                        Colors::TEXT_COLOR);

    constexpr int LINE_HEIGHT = 18;
    constexpr int PADDING_LEFT = 2;
    constexpr int PADDING_TOP = 4;

    int offset = 0;
    _textureLine1->render(renderer, PADDING_LEFT, PADDING_TOP);
    offset += LINE_HEIGHT;
    _textureLine2->render(renderer, PADDING_LEFT, PADDING_TOP + offset);
    offset += LINE_HEIGHT;
    _textureXP->render(renderer, PADDING_LEFT, PADDING_TOP + offset);
    offset += LINE_HEIGHT;
    _textureMoney->render(renderer, PADDING_LEFT, PADDING_TOP + offset);
}
