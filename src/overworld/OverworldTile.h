#pragma once

#include <SDL.h>
#include <memory>
#include "../common/graphics/PixelDecodeStrategy.h"
#include "OverworldSpriteType.h"
#include "../common/Tile.h"

using namespace std;

class OverworldTile : public Tile {
public:
    OverworldTile(int x, int y, shared_ptr<OverworldSpriteType> sprite, shared_ptr<TileAnimation> tileAnimation);

    void draw(SDL_Renderer *renderer, SDL_Rect camera) override;

    void update(float elapsed);

    OverworldSpriteType::SpriteType getSpriteType() { return _sprite->getType(); }

    // 切換 tileset 後重新指向新 tileset 的同型 sprite(避免 NPC 保留舊外觀或消失)
    void setSprite(shared_ptr<OverworldSpriteType> sprite) { _sprite = std::move(sprite); }

private:
    shared_ptr<OverworldSpriteType> _sprite;
    shared_ptr<TileAnimation> _tileAnimation;
};