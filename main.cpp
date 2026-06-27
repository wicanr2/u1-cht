#include <SDL.h>
#include <SDL_image.h>
#include <cstdio>
#include "src/overworld/OverworldTile.h"
#include "src/common/graphics/CGALinearDecodeStrategy.h"
#include "src/common/graphics/EGARowPlanarDecodeStrategy.h"
#include <memory>
#include "src/common/LTimer.h"
#include "src/overworld/OverworldScreen.h"
#include "src/common/Fonts.h"
#include "src/common/Audio.h"
#include "src/common/graphics/LTexture.h"
#include "src/Constants.h"
#include "src/CommandDisplay.h"
#include "src/dungeon/DungeonScreen.h"
#include "src/town/TownManager.h"
#include "src/town/TownScreen.h"
#include "src/town/TownSpriteTypeLoader.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include "src/Configuration.h"

using namespace std;
using namespace OpenUltima;

//Starts up SDL and creates window
bool init();

//Loads media
bool loadMedia();

//Frees media and shuts down SDL
void close();

//The window we'll be rendering to
SDL_Window *gWindow = nullptr;

//The window renderer
SDL_Renderer *gRenderer = nullptr;

// 拉高內部畫布:世界(tile/sprite)先畫進原生 320x200 離屏 target,
// 整張用 nearest 放大到 640x400,讓中文 UI 有 2x 空間清晰繪製(retro-cjk-hires-canvas)。
constexpr int GAME_W = 320, GAME_H = 200;
constexpr int CANVAS_SCALE = 2;
constexpr int CANVAS_W = GAME_W * CANVAS_SCALE, CANVAS_H = GAME_H * CANVAS_SCALE;
SDL_Texture *gWorldTarget = nullptr;

// 離開確認對話框:置中 modal + 半透明 scrim(640x400 覆蓋層空間繪製)
static void drawQuitDialog(SDL_Renderer *renderer) {
    // 暗化背景
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xB0);
    SDL_Rect scrim = {0, 0, CANVAS_W, CANVAS_H};
    SDL_RenderFillRect(renderer, &scrim);

    // 對話框
    const int bw = 360, bh = 130;
    SDL_Rect box = {(CANVAS_W - bw) / 2, (CANVAS_H - bh) / 2, bw, bh};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
    SDL_RenderFillRect(renderer, &box);
    SDL_SetRenderDrawColor(renderer, 0x40, 0x80, 0xFF, 0xFF);  // 藍邊
    SDL_RenderDrawRect(renderer, &box);

    auto line = [&](const string &s, int y, SDL_Color c) {
        LTexture t;
        t.loadFromRenderedText(Fonts::cjkUi(), renderer, s, c);
        t.render(renderer, box.x + (bw - t.getWidth()) / 2, y);
    };
    line("確定要離開遊戲嗎?", box.y + 22, SDL_Color{0xFF, 0xD0, 0x40, 0xFF});   // accent 標題
    line("Y / Enter — 離開", box.y + 60, SDL_Color{0xFF, 0xFF, 0xFF, 0xFF});
    line("N / ESC — 取消", box.y + 88, SDL_Color{0xC0, 0xC0, 0xC0, 0xFF});
}

// F6 設定選單:即時調 speed_pct / spawn_pct(置中 modal,640x400 覆蓋層)
static void drawSettingsMenu(SDL_Renderer *renderer, int row) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xB0);
    SDL_Rect scrim = {0, 0, CANVAS_W, CANVAS_H};
    SDL_RenderFillRect(renderer, &scrim);

    const int bw = 420, bh = 170;
    SDL_Rect box = {(CANVAS_W - bw) / 2, (CANVAS_H - bh) / 2, bw, bh};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
    SDL_RenderFillRect(renderer, &box);
    SDL_SetRenderDrawColor(renderer, 0x40, 0x80, 0xFF, 0xFF);
    SDL_RenderDrawRect(renderer, &box);

    auto line = [&](const string &s, int x, int y, SDL_Color c) {
        LTexture t; t.loadFromRenderedText(Fonts::cjkUi(), renderer, s, c);
        t.render(renderer, x, y);
    };
    SDL_Color title{0xFF, 0xD0, 0x40, 0xFF}, on{0xFF, 0xFF, 0x60, 0xFF}, off{0xC0, 0xC0, 0xC0, 0xFF};
    line("設定(F6)", box.x + (bw - 120) / 2, box.y + 16, title);
    line((row == 0 ? "▶ " : "  ") + ("遊戲速度: " + to_string(Configuration::getSpeedPct()) + "%"),
         box.x + 40, box.y + 60, row == 0 ? on : off);
    line((row == 1 ? "▶ " : "  ") + ("怪物生成: " + to_string(Configuration::getSpawnPct()) + "%"),
         box.x + 40, box.y + 92, row == 1 ? on : off);
    line("↑↓ 選擇   ←→ 調整   F6/ESC 關閉", box.x + 36, box.y + 132, off);
}

shared_ptr<PlayerStatusDisplay> _playerStatusDisplay;
shared_ptr<CommandDisplay> _commandDisplay;

bool init() {
    //Initialization flag
    bool success = true;

    //Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        success = false;
    } else {
        //Set texture filtering to linear
        if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 0)) {
            printf("Warning: Linear texture filtering not enabled!");
        }

        //Create window
        gWindow = SDL_CreateWindow("創世紀 I:黑暗紀元 (Ultima I) — 繁中", SDL_WINDOWPOS_UNDEFINED,
                                   SDL_WINDOWPOS_UNDEFINED, Configuration::getScreenWidth(),
                                   Configuration::getScreenHeight(), SDL_WINDOW_SHOWN);
        if (gWindow == NULL) {
            printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
            success = false;
        } else {
            //Create renderer for window
            gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            if (gRenderer == NULL) {
                printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
                success = false;
            } else {
                //Initialize renderer color
                SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

                //Initialize PNG loading
                int imgFlags = IMG_INIT_PNG;
                if (!(IMG_Init(imgFlags) & imgFlags)) {
                    printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
                    success = false;
                }
            }
        }

        //Initialize SDL_ttf
        if (TTF_Init() == -1) {
            printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
            success = false;
        }
    }

    return success;
}

bool loadMedia() {
    //Loading success flag
    bool success = true;

    //Nothing to load
    return success;
}

void close() {
    _commandDisplay = nullptr;
    _playerStatusDisplay = nullptr;

    //Destroy window
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    gWindow = NULL;
    gRenderer = NULL;

    //Quit SDL subsystems
    Audio::shutdown();
    IMG_Quit();
    SDL_Quit();
}

int main(int argc, char *args[]) {
    Configuration::init();
    cout << Configuration::getEgaOverworldTilesFilePath();

    //Start up SDL and create window
    if (!init()) {
        printf("Failed to initialize!\n");
    } else {
        //Load media
        if (!loadMedia()) {
            printf("Failed to load media!\n");
        } else {
            //Main loop flag
            bool quit = false;

            //Event handler
            SDL_Event e;

            auto player = make_shared<Player>(20, 20);
            auto gameContext = make_shared<GameContext>(player);
            // 測試 hook(env-gated,正常遊玩不啟用):直接進地牢驗證怪物移動
            if (getenv("U1_TEST_DUNGEON")) gameContext->setScreen(ScreenType::Dungeon);
            auto overworldScreen = make_shared<OverworldScreen>(gameContext, 19, 9);
            auto dungeonScreen = make_shared<DungeonScreen>(gameContext);

            auto egaTilesPath = Configuration::getEgaOverworldTilesFilePath();
            auto cgaTilesPath = Configuration::getCgaOverworldTilesFilePath();

            // tileset 變體:0=EGA(BIN),1=CGA(BIN),2..=各跨平台 PNG 素材包(F1 逐一循環)。
            // music = 該平台原版 BGM(版權,使用者自備於 assets/music/,缺檔則 fallback 占位曲)。
            struct Pack { std::string name, tiles, music; };
            const std::string kDefaultMusic = "./assets/music/theme.ogg";
            std::vector<Pack> pngPacks = {
                {"FM Towns", "assets/tilesets/fmtowns.png", "./assets/music/fmtowns.ogg"},
                {"MSX", "assets/tilesets/msx.png", "./assets/music/msx.ogg"},
                {"Apple IIgs", "assets/tilesets/iigs.png", "./assets/music/iigs.ogg"},
                {"PC-98", "assets/tilesets/pc98.png", "./assets/music/pc98.ogg"},
                {"VGA", "assets/tilesets/vga.png", kDefaultMusic},  // VGA=DOS 重染,無獨立 BGM
            };
            // config 指定的 tileset_png 若不在清單(自訂路徑)→ 沿用預設音樂。
            auto pngPackPath = Configuration::getTilesetPng();
            int cfgPngIdx = 0;
            for (size_t i = 0; i < pngPacks.size(); ++i)
                if (pngPacks[i].tiles == pngPackPath) { cfgPngIdx = (int)i; break; }
            const int kBinModes = 2;  // EGA, CGA
            const char *kBinNames[] = {"EGA", "CGA"};
            int tilesetIdx = 0;
            {
                auto ts = Configuration::getTileset();
                if (ts == "cga") tilesetIdx = 1; else if (ts == "png") tilesetIdx = kBinModes + cfgPngIdx;
            }
            int tilesetCount = kBinModes + (int)pngPacks.size();
            auto tilesetName = [&](int idx) {
                return idx >= kBinModes ? pngPacks[idx - kBinModes].name : std::string(kBinNames[idx]);
            };
            // 音樂跟隨平台:平台 BGM 載入失敗(缺檔)→ 退回占位曲,不中斷。
            auto applyMusic = [&](int idx) {
                std::string m = idx >= kBinModes ? pngPacks[idx - kBinModes].music : kDefaultMusic;
                if (!Audio::playMusic(m)) Audio::playMusic(kDefaultMusic);
            };
            auto applyTileset = [&](int idx) {
                if (idx >= kBinModes)
                    overworldScreen->initFromPng(gRenderer, pngPacks[idx - kBinModes].tiles);
                else if (idx == 1)
                    overworldScreen->init(gRenderer, make_unique<CGALinearDecodeStrategy>(16, 16).get(), cgaTilesPath);
                else
                    overworldScreen->init(gRenderer, make_unique<EGARowPlanarDecodeStrategy>(16, 16).get(), egaTilesPath);
            };

            auto townScreen = make_shared<TownScreen>(gameContext, gRenderer);

            applyTileset(tilesetIdx);

            // 螢幕邏輯畫布 = 放大後 640x400;世界另畫進 320x200 離屏 target。
            SDL_RenderSetLogicalSize(gRenderer, CANVAS_W, CANVAS_H);
            gWorldTarget = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_RGBA8888,
                                             SDL_TEXTUREACCESS_TARGET, GAME_W, GAME_H);

            //Keeps track of time between steps
            LTimer stepTimer;
            Fonts::init(gRenderer);
            Audio::init();
            applyMusic(tilesetIdx);  // 初始音樂跟隨起始平台

            _playerStatusDisplay = make_shared<PlayerStatusDisplay>(gRenderer, player);
            _commandDisplay = make_shared<CommandDisplay>(gRenderer);
            SDL_Rect commandDisplayBox = {CommandDisplay::POSITION_X, CommandDisplay::POSITION_Y, CommandDisplay::WIDTH,
                                          CommandDisplay::HEIGHT};

            shared_ptr<Dungeon> dungeon = make_shared<Dungeon>("My pet dungeon");
            dungeon->randomize();
            dungeonScreen->setDungeon(dungeon);

            shared_ptr<Screen> currentScreen = static_pointer_cast<Screen>(overworldScreen);

            //While application is running
            bool quitDialogActive = false;
            bool settingsActive = false;
            int settingsRow = 0;
            while (!quit) {
                //Handle events on queue
                while (SDL_PollEvent(&e) != 0) {
                    // F6 設定選單開啟時:↑↓選列、←→±5%、F6/ESC 關閉,吞掉其他輸入
                    if (settingsActive) {
                        if (e.type == SDL_KEYDOWN) {
                            auto k = e.key.keysym.sym;
                            if (k == SDLK_F6 || k == SDLK_ESCAPE) settingsActive = false;
                            else if (k == SDLK_UP) settingsRow = 0;
                            else if (k == SDLK_DOWN) settingsRow = 1;
                            else if (k == SDLK_LEFT || k == SDLK_RIGHT) {
                                int d = (k == SDLK_RIGHT) ? 5 : -5;
                                if (settingsRow == 0) Configuration::setSpeedPct(Configuration::getSpeedPct() + d);
                                else Configuration::setSpawnPct(Configuration::getSpawnPct() + d);
                            }
                        }
                        continue;
                    }

                    // 離開語意鐵則:ESC 只取消,F10/Ctrl+Q(及關窗)才彈離開確認
                    if (quitDialogActive) {
                        if (e.type == SDL_KEYDOWN) {
                            auto k = e.key.keysym.sym;
                            if (k == SDLK_y || k == SDLK_RETURN) {
                                // TODO: 存檔系統建立後在此 autosave 再離開
                                quit = true;
                            } else if (k == SDLK_n || k == SDLK_ESCAPE) {
                                quitDialogActive = false;
                            }
                        }
                        continue;  // dialog 開啟時吞掉其他輸入
                    }

                    //User requests quit
                    if (e.type == SDL_QUIT) {
                        quitDialogActive = true;  // 關窗也走確認流程
                    } else {
                        if (e.type == SDL_KEYDOWN) {
                            auto pressedKey = e.key.keysym.sym;
                            bool ctrl = (e.key.keysym.mod & KMOD_CTRL) != 0;
                            if (pressedKey == SDLK_F10 || (ctrl && pressedKey == SDLK_q)) {
                                quitDialogActive = true;
                                continue;
                            }
                            if (pressedKey == SDLK_F6) {
                                settingsActive = true;
                                continue;
                            }
                            // F1 或 PageDown:循環切換 tileset(EGA→CGA→PNG包)+ 中文提示。
                            if (pressedKey == SDLK_PAGEDOWN || pressedKey == SDLK_F1) {
                                tilesetIdx = (tilesetIdx + 1) % tilesetCount;
                                applyTileset(tilesetIdx);
                                applyMusic(tilesetIdx);  // 音樂跟隨切換的平台
                                CommandDisplay::writeLn(string("圖形模式:") + tilesetName(tilesetIdx), false);
                            }
                            // M:切換背景音樂
                            if (pressedKey == SDLK_m) {
                                if (Audio::available()) {
                                    bool on = Audio::toggleMute();
                                    CommandDisplay::writeLn(on ? "音樂:開" : "音樂:關", false);
                                } else {
                                    CommandDisplay::writeLn("音樂:無音訊裝置", false);
                                }
                            }
                        }

                        currentScreen->handle(e);
                    }
                }

                //Calculate time step
                float timeStep = stepTimer.getTicks();

                //Restart step timer
                stepTimer.start();

                // === 世界繪製:畫進原生 320x200 離屏 target ===
                SDL_SetRenderTarget(gRenderer, gWorldTarget);
                SDL_RenderSetLogicalSize(gRenderer, GAME_W, GAME_H);
                SDL_SetRenderDrawColor(gRenderer, 0, 0, 0xAF, 0);
                SDL_RenderClear(gRenderer);

                switch (gameContext->getCurrentScreen()) {
                    case ScreenType::Overworld:
                        currentScreen = static_pointer_cast<Screen>(overworldScreen);
                        break;
                    case ScreenType::Dungeon:
                        currentScreen = static_pointer_cast<Screen>(dungeonScreen);
                        break;
                    case ScreenType::Town:
                        if (currentScreen != townScreen) {
                            townScreen->refresh();
                        }
                        currentScreen = static_pointer_cast<Screen>(townScreen);
                        break;
                    case ScreenType::Space:
                        throw runtime_error("Unhandled");
                }

                currentScreen->update(timeStep);

                SDL_Rect defaultViewport = {0, 0, GAME_W, GAME_H};
                SDL_RenderSetViewport(gRenderer, &defaultViewport);

                currentScreen->draw(gRenderer);

                // === 放大世界 target 到螢幕 640x400 (nearest, crisp) ===
                SDL_SetRenderTarget(gRenderer, nullptr);
                SDL_RenderSetLogicalSize(gRenderer, CANVAS_W, CANVAS_H);
                SDL_RenderSetViewport(gRenderer, nullptr);
                SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 0xFF);
                SDL_RenderClear(gRenderer);
                SDL_RenderCopy(gRenderer, gWorldTarget, nullptr, nullptr);

                // === UI(中文)在 640x400 高解析空間繪製,座標 x2 ===
                _playerStatusDisplay->draw(gRenderer);

                SDL_Rect commandBoxHi = {commandDisplayBox.x * CANVAS_SCALE, commandDisplayBox.y * CANVAS_SCALE,
                                         commandDisplayBox.w * CANVAS_SCALE, commandDisplayBox.h * CANVAS_SCALE};
                SDL_RenderSetViewport(gRenderer, &commandBoxHi);
                _commandDisplay->draw(gRenderer);

                // 設定選單 / 離開確認對話框(覆蓋最上層)
                if (settingsActive) {
                    SDL_RenderSetViewport(gRenderer, nullptr);
                    drawSettingsMenu(gRenderer, settingsRow);
                }
                if (quitDialogActive) {
                    SDL_RenderSetViewport(gRenderer, nullptr);
                    drawQuitDialog(gRenderer);
                }

                //Update screen
                SDL_RenderPresent(gRenderer);
            }
        }
    }

    //Free resources and close SDL
    close();

    return 0;
}