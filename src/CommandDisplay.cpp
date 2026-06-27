#include "CommandDisplay.h"
#include "common/Fonts.h"
#include "common/Colors.h"
#include "common/ShapeUtils.h"

vector<tuple<string, bool>> CommandDisplay::_text;
string CommandDisplay::_promptText = "";

// 解一個 UTF-8 字元 → (codepoint, byte 長度)
static pair<unsigned, int> decodeUtf8(const string &s, size_t i) {
    unsigned char c = s[i];
    if (c < 0x80) return {c, 1};
    if ((c & 0xE0) == 0xC0 && i + 1 < s.size())
        return {((c & 0x1Fu) << 6) | (s[i + 1] & 0x3Fu), 2};
    if ((c & 0xF0) == 0xE0 && i + 2 < s.size())
        return {((c & 0x0Fu) << 12) | ((s[i + 1] & 0x3Fu) << 6) | (s[i + 2] & 0x3Fu), 3};
    if ((c & 0xF8) == 0xF0 && i + 3 < s.size())
        return {0xFFFFu, 4};   // 視為全形(BMP 外,罕用)
    return {c, 1};
}

// 顯示寬度:全形(CJK/假名/全形標點與英數/Hangul)= 2,其餘 = 1
static int charWidth(unsigned cp) {
    if ((cp >= 0x1100 && cp <= 0x115F) || (cp >= 0x2E80 && cp <= 0xA4CF) ||
        (cp >= 0xAC00 && cp <= 0xD7A3) || (cp >= 0xF900 && cp <= 0xFAFF) ||
        (cp >= 0xFE30 && cp <= 0xFE4F) || (cp >= 0xFF00 && cp <= 0xFF60) ||
        (cp >= 0xFFE0 && cp <= 0xFFE6) || cp == 0xFFFF)
        return 2;
    return 1;
}

CommandDisplay::CommandDisplay(SDL_Renderer *renderer) {
    _background = make_unique<LTexture>();
    _background->loadFromColor(renderer, WIDTH, HEIGHT, 0, 0, 0, 0);

    _line1 = make_unique<LTexture>();
    _line2 = make_unique<LTexture>();
    _line3 = make_unique<LTexture>();
    _promptLine = make_unique<LTexture>();

    _actionIcon = ShapeUtils::arrow(renderer, ShapeUtils::Direction::Right);
}

void CommandDisplay::writeLn(const string &str, bool isPlayerAction) {
    if (_promptText.empty()) {
        // 依「顯示寬度」斷行(全形 2、半形 1),且只在 UTF-8 字元邊界切,中文不被切壞。
        vector<string> substrs;
        size_t i = 0;
        int lineWidth = 0;
        size_t lineStart = 0;
        while (i < str.size()) {
            auto [cp, len] = decodeUtf8(str, i);
            int w = charWidth(cp);
            if (lineWidth + w > MAX_LINE_WIDTH && i > lineStart) {
                substrs.push_back(str.substr(lineStart, i - lineStart));
                lineStart = i;
                lineWidth = 0;
            }
            lineWidth += w;
            i += len;
        }
        if (lineStart < str.size() || substrs.empty())
            substrs.push_back(str.substr(lineStart));

        for (const auto &substr : substrs) {
            _text.emplace_back(substr, isPlayerAction);
        }
    } else {
        _text.emplace_back(_promptText + str, isPlayerAction);
        _promptText = "";
    }

    if (_text.size() > MAX_LINES) {
        _text.erase(_text.begin());
    }
}

void CommandDisplay::write(const string &str) {
    _promptText = str;
}

void CommandDisplay::draw(SDL_Renderer *renderer) {
    // 黑底面板填滿 x2 viewport(取代未縮放的 _background 貼圖)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
    SDL_Rect bg = {0, 0, WIDTH * 2, HEIGHT * 2};
    SDL_RenderFillRect(renderer, &bg);

    int lineIndex = _text.size() - 1;

    // 640 高解析覆蓋層:行高/邊距 x2,字型 16px(viewport 已於 main x2)
    constexpr int LINE_HEIGHT = 18;
    constexpr int PADDING_LEFT = 16;
    constexpr int PADDING_TOP = 6;
    constexpr int PADDING_FONT = 4;

    int offset = LINE_HEIGHT * 2;

    if (lineIndex >= 0) {
        auto[line, playerAction] = _text.at(lineIndex--);
        _line3->loadFromRenderedText(Fonts::cjkUi(), renderer, line, Colors::TEXT_COLOR);
        if (playerAction) {
            _actionIcon->render(renderer, 0, offset + PADDING_TOP + PADDING_FONT);
        }
        _line3->render(renderer, PADDING_LEFT, offset + PADDING_TOP);
        offset -= LINE_HEIGHT;
    }

    if (lineIndex >= 0) {
        auto[line, playerAction] = _text.at(lineIndex--);
        _line2->loadFromRenderedText(Fonts::cjkUi(), renderer, line, Colors::TEXT_COLOR);
        if (playerAction) {
            _actionIcon->render(renderer, 0, offset + PADDING_TOP + PADDING_FONT);
        }
        _line2->render(renderer, PADDING_LEFT, offset + PADDING_TOP);
        offset -= LINE_HEIGHT;
    }

    if (lineIndex >= 0) {
        auto[line, playerAction] = _text.at(lineIndex--);
        _line1->loadFromRenderedText(Fonts::cjkUi(), renderer, line, Colors::TEXT_COLOR);
        if (playerAction) {
            _actionIcon->render(renderer, 0, offset + PADDING_TOP + PADDING_FONT);
        }
        _line1->render(renderer, PADDING_LEFT, offset + PADDING_TOP);
    }

    _actionIcon->render(renderer, 0, PADDING_TOP + LINE_HEIGHT * 3 + PADDING_FONT);
    if (!_promptText.empty()) {
        _promptLine->loadFromRenderedText(Fonts::cjkUi(), renderer, _promptText, Colors::TEXT_COLOR);
        _promptLine->render(renderer, PADDING_LEFT, PADDING_TOP + LINE_HEIGHT * 3);
    }
}