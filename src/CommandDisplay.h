#pragma once

#include "common/graphics/LTexture.h"
#include <memory>
#include "Player.h"
#include <vector>
#include <string>

using namespace std;

class CommandDisplay {
public:
    static constexpr int POSITION_X = 0;
    static constexpr int POSITION_Y = 160;
    static constexpr int WIDTH = 241;
    static constexpr int HEIGHT = 40;
    // 換行以「顯示寬度」計:全形(中文/假名/全形標點)= 2 單位、半形 = 1 單位。
    // panel 約可容 28 個全形字 → 上限 ~54 半形單位(留邊距取 52)。
    static constexpr int MAX_LINE_WIDTH = 52;

    static void writeLn(const string &str, bool isPlayerAction);

    static void write(const string &str);

    explicit CommandDisplay(SDL_Renderer *renderer);

    void draw(SDL_Renderer *renderer);

private:
    static constexpr int MAX_LINES = 3;

    static vector<tuple<string, bool>> _text;
    static string _promptText;

    unique_ptr<LTexture> _background;
    shared_ptr<LTexture> _actionIcon;
    unique_ptr<LTexture> _line1;
    unique_ptr<LTexture> _line2;
    unique_ptr<LTexture> _line3;
    unique_ptr<LTexture> _promptLine;
};

