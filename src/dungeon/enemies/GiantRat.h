#pragma once

#include <SDL2/SDL_render.h>
#include "../../common/I18n.h"
#include "Enemy.h"

class GiantRat : public Enemy {
public:
    GiantRat(int x, int y) : Enemy(x, y, 5) {}

    void draw(SDL_Renderer *renderer, int distance) override;

    void update(float elapsed) override;

    string getName() override { return I18n::t("monster.giant_rat"); };
};


