#include "CommandDisplay.h"
#include "common/Fonts.h"
#include "common/Colors.h"
#include "common/ShapeUtils.h"

vector<tuple<string, bool>> CommandDisplay::_text;
string CommandDisplay::_promptText = "";

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
        vector<string> substrs;

        int remainingLength = str.length();
        if (remainingLength > MAX_LINE_CHARS) {
            int index = 0;
            while (remainingLength > 0) {
                auto substrLength = min(MAX_LINE_CHARS - 1, remainingLength);
                // UTF-8 安全:切點落在 continuation byte (0b10xxxxxx) 時往前退到字元邊界,
                // 避免把多位元組中文字切壞成無效 UTF-8(暫行;全形換行於拉畫布階段完整處理)
                while (substrLength < remainingLength &&
                       (static_cast<unsigned char>(str[index + substrLength]) & 0xC0) == 0x80) {
                    substrLength--;
                }
                auto substr = str.substr(index, substrLength);
                substrs.push_back(" " + substr);
                remainingLength -= substrLength;
                index += substrLength;
            }
        } else {
            substrs.push_back(str);
        }

        for (const auto &substr: substrs) {
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