#pragma once
#include <SDL.h>
#include <SDL_ttf.h>
class Fonts
{
public:
	static void init(SDL_Renderer* renderer);
	// ASCII 原版點陣字(8px,沿用上游 PC_Senior)
	static TTF_Font* standard() { return _standard; };
	// CJK 中文字(Noto CJK,12px);in-target(320空間)文字用,載入失敗回退 standard
	static TTF_Font* cjk() { return _cjk ? _cjk : _standard; };
	// CJK UI 字(16px);640 高解析覆蓋層(狀態列/指令列)用
	static TTF_Font* cjkUi() { return _cjkUi ? _cjkUi : cjk(); };
private:
	static TTF_Font* _standard;
	static TTF_Font* _cjk;
	static TTF_Font* _cjkUi;
};
