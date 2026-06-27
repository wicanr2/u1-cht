#pragma once

#include <SDL2/SDL_render.h>
#include "../../common/I18n.h"
#include "Enemy.h"

class Thief : public Enemy {
public:
    Thief(int x, int y) : Enemy(x, y, 10) {}

    void draw(SDL_Renderer *renderer, int distance) override;

    void update(float elapsed) override;

    string getName() override { return I18n::t("monster.thief"); };
};


