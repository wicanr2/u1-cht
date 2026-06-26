#pragma once
#include <SDL.h>
#include <SDL_ttf.h>
class Fonts
{
public:
	static void init(SDL_Renderer* renderer);
	// ASCII 原版點陣字(8px,沿用上游 PC_Senior)
	static TTF_Font* standard() { return _standard; };
	// CJK 中文字(Noto CJK);中文化文字走此字型,載入失敗則回退 standard
	static TTF_Font* cjk() { return _cjk ? _cjk : _standard; };
private:
	static TTF_Font* _standard;
	static TTF_Font* _cjk;
};
