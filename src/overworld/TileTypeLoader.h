#pragma once

#include <string>
#include "../common/graphics/PixelDecodeStrategy.h"
#include "OverworldSpriteType.h"
#include "../common/graphics/LTexture.h"

using namespace std;

class TileTypeLoader {
public:
    static vector<shared_ptr<OverworldSpriteType>>
    loadOverworldSprites(const string &tilesFileLocation, PixelDecodeStrategy *pixelDecodeStrategy,
                         SDL_Renderer *renderer);

    // PNG AssetPack:從 PNG sprite sheet(832×16 水平長條)載入 overworld sprite
    static vector<shared_ptr<OverworldSpriteType>>
    loadOverworldSpritesFromPng(const string &pngPath, SDL_Renderer *renderer);

    // 共用:給定 tileset texture,建出 52 個 OverworldSpriteType(index→type 對應)
    static vector<shared_ptr<OverworldSpriteType>>
    buildSprites(shared_ptr<LTexture> overworldTexture);
};