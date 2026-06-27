#pragma once

#include "../Screen.h"
#include <vector>

// U1 星海試煉(Starwalking):駕太空梭在星海擊落與蒙登結盟的敵艦,
// 達標即成為「Space Ace」(殺蒙登的前置門檻)。線框/向量風格,SDL 基本圖元繪製。
// 簡化但忠於原版精神:玩家船底部,敵艦自上方逼近,射擊累積擊落數達標即過關。
class SpaceScreen : public Screen {
public:
    explicit SpaceScreen(shared_ptr<GameContext> gameContext);

    void update(float elapsed) override;
    void draw(SDL_Renderer *renderer) override;
    void handle(const SDL_Event &e) override;

    void reset();   // 進入星海時重置

    static constexpr int KILLS_TO_ACE = 15;   // 成為 Space Ace 所需擊落數

private:
    struct P { float x, y; };

    float _shipX = 152.0f;
    std::vector<P> _bullets;
    std::vector<P> _enemies;
    std::vector<P> _stars;
    int _kills = 0;
    float _spawnTimer = 0.0f;
    float _fireCooldown = 0.0f;
    bool _initialized = false;

    void fire();
    void initStars();
};
