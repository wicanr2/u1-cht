#pragma once

#include <SDL2/SDL_render.h>
#include <string>

using namespace std;

class Enemy {
public:
    Enemy(int x, int y, int hp) : _x(x), _y(y), _hp(hp) {}
    Enemy(int x, int y, int hp, int damage) : _x(x), _y(y), _hp(hp), _damage(damage) {}

    virtual void update(float elapsed) = 0;

    virtual void draw(SDL_Renderer *renderer, int distance) = 0;

    virtual string getName() = 0;

    int getX() { return _x; }

    int getY() { return _y; }

    // 地牢怪移動用(忠於原版 U1:地牢怪逐玩家移動)
    void setPosition(int x, int y) { _x = x; _y = y; }

    int getHP() { return _hp; }

    int getDamage() { return _damage; }

    void receiveDamage(int damage);

    bool isDead() { return _hp <= 0; }

protected:
    int _x;
    int _y;
    int _hp;
    int _damage = 1;   // 基礎傷害(deeper bestiary 怪可設更高)
};


