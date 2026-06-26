#include "Fonts.h"
#include <cstdio>

TTF_Font* Fonts::_standard = nullptr;
TTF_Font* Fonts::_cjk = nullptr;
TTF_Font* Fonts::_cjkUi = nullptr;

// CJK 字型候選路徑:優先用 repo 內重建字庫,否則回退系統 Noto CJK。
static const char* kCjkFontCandidates[] = {
    "./assets/fonts/NotoSansCJK-Regular.ttc",
    "./assets/fonts/u1-cjk.ttf",
    "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
};

void Fonts::init(SDL_Renderer* renderer) {
    _standard = TTF_OpenFont("./resources/PC_Senior_Regular.ttf", 8);

    // CJK:cjk()=12px(in-target 320 空間);cjkUi()=16px(640 高解析覆蓋層)
    for (auto path : kCjkFontCandidates) {
        _cjk = TTF_OpenFont(path, 12);
        _cjkUi = TTF_OpenFont(path, 16);
        if (_cjk) {
            printf("[Fonts] CJK font loaded: %s\n", path);
            break;
        }
    }
    if (!_cjk) {
        printf("[Fonts] WARN: no CJK font found, falling back to ASCII standard\n");
    }
}
