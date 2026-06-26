#pragma once

#include <vector>
#include <SDL_render.h>
#include <memory>
#include <string>

#include "PixelDecodeStrategy.h"
#include "LTexture.h"

using namespace std;

class SpriteSheetLoader {
public:
    static shared_ptr<LTexture> loadTexture(SDL_RWops* file, PixelDecodeStrategy* pixelDecodeStrategy, SDL_Renderer* renderer, int spriteCount, int spriteWidth, int spriteHeight);

    // PNG AssetPack:直接載入已是「水平長條(832×16)」排列的 PNG sprite sheet 當 tileset texture
    static shared_ptr<LTexture> loadTextureFromPng(const string& path, SDL_Renderer* renderer);
};