#include "PlayerStatusDisplay.h"

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
    SDL_Rect viewport = {PlayerStatusDisplay::POSITION_X, PlayerStatusDisplay::POSITION_Y,
                         PlayerStatusDisplay::WIDTH, PlayerStatusDisplay::HEIGHT};
    SDL_RenderSetViewport(renderer, &viewport);

    _background->render(renderer, 0, 0);

    _textureLine1->loadFromRenderedText(Fonts::cjk(), renderer, "生命: " + to_string(_player->getHP()),
                                        Colors::TEXT_COLOR);
    _textureLine2->loadFromRenderedText(Fonts::cjk(), renderer, "食物: " + to_string(_player->getFood()),
                                        Colors::TEXT_COLOR);
    _textureXP->loadFromRenderedText(Fonts::cjk(), renderer, "經驗: " + to_string(_player->getXP()),
                                     Colors::TEXT_COLOR);
    _textureMoney->loadFromRenderedText(Fonts::cjk(), renderer, "金幣: " + to_string(_player->getMoney()),
                                        Colors::TEXT_COLOR);

    constexpr int LINE_HEIGHT = 8;
    constexpr int PADDING_LEFT = 1;
    constexpr int PADDING_TOP = 6;

    int offset = 0;
    _textureLine1->render(renderer, PADDING_LEFT, PADDING_TOP);
    offset += LINE_HEIGHT;
    _textureLine2->render(renderer, PADDING_LEFT, PADDING_TOP + offset);
    offset += LINE_HEIGHT;
    _textureXP->render(renderer, PADDING_LEFT, PADDING_TOP + offset);
    offset += LINE_HEIGHT;
    _textureMoney->render(renderer, PADDING_LEFT, PADDING_TOP + offset);
}
