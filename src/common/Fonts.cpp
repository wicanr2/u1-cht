#include "Fonts.h"
#include <cstdio>

TTF_Font* Fonts::_standard = nullptr;
TTF_Font* Fonts::_cjk = nullptr;

// CJK 字型候選路徑:優先用 repo 內重建字庫,否則回退系統 Noto CJK。
static const char* kCjkFontCandidates[] = {
    "./assets/fonts/NotoSansCJK-Regular.ttc",
    "./assets/fonts/u1-cjk.ttf",
    "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
};

void Fonts::init(SDL_Renderer* renderer) {
    _standard = TTF_OpenFont("./resources/PC_Senior_Regular.ttf", 8);

    // CJK 字級略大於 8px 以維持中文可讀(細節隨拉畫布策略再調)
    for (auto path : kCjkFontCandidates) {
        _cjk = TTF_OpenFont(path, 12);
        if (_cjk) {
            printf("[Fonts] CJK font loaded: %s\n", path);
            break;
        }
    }
    if (!_cjk) {
        printf("[Fonts] WARN: no CJK font found, falling back to ASCII standard\n");
    }
}
