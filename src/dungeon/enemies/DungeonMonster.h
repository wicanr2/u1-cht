#pragma once

#include <SDL2/SDL_render.h>
#include <string>
#include "../../common/I18n.h"
#include "Enemy.h"

// 參數化地牢怪:名稱(i18n key)/ HP / 傷害 / 顏色 / 外形 一律由建構參數決定,
// 用可縮放的線框剪影 draw(by distance)取代為每種怪手繪。讓整套 bestiary 一個類別搞定。
class DungeonMonster : public Enemy {
public:
    enum class Shape { Humanoid, Beast, Blob, Flyer };

    DungeonMonster(int x, int y, int hp, int damage, std::string nameKey,
                   Shape shape, Uint8 r, Uint8 g, Uint8 b)
        : Enemy(x, y, hp, damage), _nameKey(std::move(nameKey)),
          _shape(shape), _r(r), _g(g), _b(b) {}

    void draw(SDL_Renderer *renderer, int distance) override;
    void update(float) override {}
    std::string getName() override { return I18n::t(_nameKey); }

private:
    std::string _nameKey;
    Shape _shape;
    Uint8 _r, _g, _b;
};
