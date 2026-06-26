#include "SpriteSheetLoader.h"
#include <SDL_image.h>
#include <cstdio>

shared_ptr<LTexture> SpriteSheetLoader::loadTextureFromPng(const string &path, SDL_Renderer *renderer) {
    SDL_Surface *surface = IMG_Load(path.c_str());
    if (!surface) {
        printf("[SpriteSheet] PNG load failed (%s): %s\n", path.c_str(), IMG_GetError());
        return nullptr;
    }
    int w = surface->w, h = surface->h;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return make_shared<LTexture>(tex, w, h);
}

shared_ptr<LTexture> SpriteSheetLoader::loadTexture(SDL_RWops *file, PixelDecodeStrategy *pixelDecodeStrategy,
                                                    SDL_Renderer *renderer,
                                                    int spriteCount, int spriteWidth, int spriteHeight) {
    auto tileBytesCount = pixelDecodeStrategy->getBytesPerTile();
    shared_ptr<uint8_t[]> buffer(new uint8_t[tileBytesCount * spriteCount]);

    SDL_RWread(file, buffer.get(), 1, tileBytesCount * spriteCount);

    int textureWidth = spriteWidth * spriteCount;
    int textureHeight = spriteHeight;

    auto sdlTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, textureWidth,
                                        textureHeight);
    auto texture = make_shared<LTexture>(sdlTexture, textureWidth, textureHeight);

    auto startIndex = 0;
    for (int spriteIndex = 0; spriteIndex < spriteCount; spriteIndex++) {
        auto rawPointer = buffer.get();
        auto spriteBytes = vector(rawPointer + startIndex, rawPointer + startIndex + tileBytesCount);
        auto pixels = pixelDecodeStrategy->GetPixels(spriteBytes);

        SDL_Rect target = {spriteIndex * spriteWidth, 0, spriteWidth, spriteHeight};

        SDL_UpdateTexture(sdlTexture, &target, pixels.data(), spriteWidth * sizeof(uint32_t));
        startIndex += tileBytesCount;
    }

    return texture;
}
