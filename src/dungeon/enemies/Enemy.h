#pragma once

#include <SDL2/SDL_render.h>
#include <string>

using namespace std;

class Enemy {
public:
    Enemy(int x, int y, int hp) : _x(x), _y(y), _hp(hp) {}

    virtual void update(float elapsed) = 0;

    virtual void draw(SDL_Renderer *renderer, int distance) = 0;

    virtual string getName() = 0;

    int getX() { return _x; }

    int getY() { return _y; }

    // 地牢怪移動用(忠於原版 U1:地牢怪逐玩家移動)
    void setPosition(int x, int y) { _x = x; _y = y; }

    int getHP() { return _hp; }

    int getDamage() { return 1; }

    void receiveDamage(int damage);

    bool isDead() { return _hp <= 0; }

protected:
    int _x;
    int _y;
    int _hp;
};


