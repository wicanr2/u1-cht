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
#include "src/SaveGame.h"
#include "src/common/I18n.h"

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
    line(I18n::t("ui.quit.confirm"), box.y + 22, SDL_Color{0xFF, 0xD0, 0x40, 0xFF});   // accent 標題
    line(I18n::t("ui.quit.yes"), box.y + 60, SDL_Color{0xFF, 0xFF, 0xFF, 0xFF});
    line(I18n::t("ui.quit.no"), box.y + 88, SDL_Color{0xC0, 0xC0, 0xC0, 0xFF});
}

// F6 設定選單:即時調 speed_pct / spawn_pct(置中 modal,640x400 覆蓋層)
static void drawSettingsMenu(SDL_Renderer *renderer, int row) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xB0);
    SDL_Rect scrim = {0, 0, CANVAS_W, CANVAS_H};
    SDL_RenderFillRect(renderer, &scrim);

    const int bw = 440, bh = 210;
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
    line(I18n::t("ui.settings.title"), box.x + (bw - 120) / 2, box.y + 16, title);
    line((row == 0 ? "▶ " : "  ") + (I18n::t("ui.settings.speed") + to_string(Configuration::getSpeedPct()) + "%"),
         box.x + 40, box.y + 56, row == 0 ? on : off);
    line((row == 1 ? "▶ " : "  ") + (I18n::t("ui.settings.spawn") + to_string(Configuration::getSpawnPct()) + "%"),
         box.x + 40, box.y + 88, row == 1 ? on : off);
    line((row == 2 ? "▶ " : "  ") + (I18n::t("ui.settings.chase") + string(Configuration::getChaseMonsters() ? I18n::t("ui.on") : I18n::t("ui.off"))),
         box.x + 40, box.y + 120, row == 2 ? on : off);
    line(I18n::t("ui.settings.hint"), box.x + 36, box.y + 168, off);
}

// F1 說明畫面:列出全部指令(置中 modal,640x400 覆蓋層)
static void drawHelpScreen(SDL_Renderer *renderer) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xC0);
    SDL_Rect scrim = {0, 0, CANVAS_W, CANVAS_H};
    SDL_RenderFillRect(renderer, &scrim);

    const int bw = 540, bh = 420;
    SDL_Rect box = {(CANVAS_W - bw) / 2, (CANVAS_H - bh) / 2, bw, bh};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
    SDL_RenderFillRect(renderer, &box);
    SDL_SetRenderDrawColor(renderer, 0x40, 0x80, 0xFF, 0xFF);
    SDL_RenderDrawRect(renderer, &box);

    SDL_Color title{0xFF, 0xD0, 0x40, 0xFF}, head{0x80, 0xC0, 0xFF, 0xFF},
        key{0xFF, 0xFF, 0x80, 0xFF}, txt{0xE0, 0xE0, 0xE0, 0xFF};
    auto line = [&](const string &s, int x, int y, SDL_Color c) {
        LTexture t; t.loadFromRenderedText(Fonts::cjkUi(), renderer, s, c);
        t.render(renderer, x, y);
    };
    int lx = box.x + 30, kx = box.x + 50, tx = box.x + 200, y = box.y + 18;
    line(I18n::t("ui.help.title"), box.x + (bw - 140) / 2, y, title); y += 38;
    line(I18n::t("ui.help.move_head"), lx, y, head); y += 28;
    line(I18n::t("ui.help.arrows"), kx, y, key); line(I18n::t("ui.help.arrows_desc"), tx, y, txt); y += 30;
    line(I18n::t("ui.help.action_head"), lx, y, head); y += 28;
    line("A", kx, y, key); line(I18n::t("ui.help.attack"), tx, y, txt); y += 26;
    line("E", kx, y, key); line(I18n::t("ui.help.enter"), tx, y, txt); y += 26;
    line("K", kx, y, key); line(I18n::t("ui.help.klimb"), tx, y, txt); y += 30;
    line(I18n::t("ui.help.sys_head"), lx, y, head); y += 28;
    line("F6", kx, y, key); line(I18n::t("ui.help.f6"), tx, y, txt); y += 24;
    line("M  /  F9", kx, y, key); line(I18n::t("ui.help.audio"), tx, y, txt); y += 24;
    line("PageDown", kx, y, key); line(I18n::t("ui.help.tileset"), tx, y, txt); y += 24;
    line("F5", kx, y, key); line(I18n::t("ui.help.save"), tx, y, txt); y += 24;
    line("F10 / Ctrl+Q", kx, y, key); line(I18n::t("ui.help.quit"), tx, y, txt); y += 24;
    line("ESC", kx, y, key); line(I18n::t("ui.help.esc"), tx, y, txt); y += 30;
    line(I18n::t("ui.help.dismiss"), box.x + (bw - 120) / 2, box.y + bh - 30, SDL_Color{0xA0, 0xA0, 0xA0, 0xFF});
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
        gWindow = SDL_CreateWindow(I18n::t("window.title").c_str(), SDL_WINDOWPOS_UNDEFINED,
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
    I18n::init("assets/strings/" + Configuration::getLanguage() + ".json");
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
            // 啟動時若有存檔則載入(還原玩家位置/數值 + F6 設定);損毀則維持新遊戲
            if (SaveGame::exists(SaveGame::defaultPath()))
                SaveGame::load(*player, SaveGame::defaultPath());
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
                {"Atari", "assets/tilesets/atari.png", "./assets/music/atari.ogg"},
                {"VGA", "assets/tilesets/vga.png", kDefaultMusic},  // VGA=DOS 重染,無獨立 BGM
            };
            // config 指定的 tileset_png:在清單則用其 index;不在(自訂路徑)→ 附加成新 pack,
            // 避免靜默 fallback 到 pngPacks[0] 而載錯 tileset。
            auto pngPackPath = Configuration::getTilesetPng();
            int cfgPngIdx = -1;
            for (size_t i = 0; i < pngPacks.size(); ++i)
                if (pngPacks[i].tiles == pngPackPath) { cfgPngIdx = (int)i; break; }
            if (cfgPngIdx < 0) {
                pngPacks.push_back({I18n::t("tileset.custom"), pngPackPath, kDefaultMusic});
                cfgPngIdx = (int)pngPacks.size() - 1;
            }
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
            bool helpActive = false;
            int settingsRow = 0;
            while (!quit) {
                //Handle events on queue
                while (SDL_PollEvent(&e) != 0) {
                    // 說明畫面開啟時:任意鍵(F1/ESC/Enter/空白)關閉,吞掉其他輸入
                    if (helpActive) {
                        if (e.type == SDL_KEYDOWN) helpActive = false;
                        continue;
                    }

                    // F6 設定選單開啟時:↑↓選列(3 列)、←→ 調整 / 切換、F6/ESC 關閉,吞掉其他輸入
                    if (settingsActive) {
                        if (e.type == SDL_KEYDOWN) {
                            auto k = e.key.keysym.sym;
                            if (k == SDLK_F6 || k == SDLK_ESCAPE) settingsActive = false;
                            else if (k == SDLK_UP) settingsRow = (settingsRow + 2) % 3;
                            else if (k == SDLK_DOWN) settingsRow = (settingsRow + 1) % 3;
                            else if (k == SDLK_LEFT || k == SDLK_RIGHT) {
                                if (settingsRow == 2) {
                                    Configuration::setChaseMonsters(!Configuration::getChaseMonsters());  // 開關
                                } else {
                                    int d = (k == SDLK_RIGHT) ? 5 : -5;
                                    if (settingsRow == 0) Configuration::setSpeedPct(Configuration::getSpeedPct() + d);
                                    else Configuration::setSpawnPct(Configuration::getSpawnPct() + d);
                                }
                            }
                        }
                        continue;
                    }

                    // 離開語意鐵則:ESC 只取消,F10/Ctrl+Q(及關窗)才彈離開確認
                    if (quitDialogActive) {
                        if (e.type == SDL_KEYDOWN) {
                            auto k = e.key.keysym.sym;
                            if (k == SDLK_y || k == SDLK_RETURN) {
                                // 離開鐵則:離開前 autosave(失敗則提示,但仍允許離開)
                                if (!SaveGame::save(*player, SaveGame::defaultPath()))
                                    CommandDisplay::writeLn(I18n::t("msg.save_fail_exit"), false);
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
                            // F1:說明畫面(列出所有指令)。
                            if (pressedKey == SDLK_F1) {
                                helpActive = true;
                                continue;
                            }
                            // PageDown:循環切換 tileset(EGA→CGA→各平台 PNG 包)+ 中文提示。
                            if (pressedKey == SDLK_PAGEDOWN) {
                                tilesetIdx = (tilesetIdx + 1) % tilesetCount;
                                applyTileset(tilesetIdx);
                                applyMusic(tilesetIdx);  // 音樂跟隨切換的平台
                                CommandDisplay::writeLn(string(I18n::t("msg.graphics_mode")) + tilesetName(tilesetIdx), false);
                            }
                            // M:切換背景音樂
                            if (pressedKey == SDLK_m) {
                                if (Audio::available()) {
                                    bool on = Audio::toggleMute();
                                    CommandDisplay::writeLn(on ? I18n::t("msg.music_on") : I18n::t("msg.music_off"), false);
                                } else {
                                    CommandDisplay::writeLn(I18n::t("msg.music_no_device"), false);
                                }
                            }
                            // F5:手動存檔
                            if (pressedKey == SDLK_F5) {
                                bool ok = SaveGame::save(*player, SaveGame::defaultPath());
                                CommandDisplay::writeLn(ok ? I18n::t("msg.saved") : I18n::t("msg.save_fail"), false);
                            }
                            // F9:切換音效(獨立於音樂)
                            if (pressedKey == SDLK_F9) {
                                if (Audio::available()) {
                                    bool on = Audio::toggleSfx();
                                    CommandDisplay::writeLn(on ? I18n::t("msg.sfx_on") : I18n::t("msg.sfx_off"), false);
                                } else {
                                    CommandDisplay::writeLn(I18n::t("msg.sfx_no_device"), false);
                                }
                            }
                            // 音效:移動踏步 / 攻擊撞擊(音效靜音時自動 no-op)
                            if (pressedKey == SDLK_UP || pressedKey == SDLK_DOWN ||
                                pressedKey == SDLK_LEFT || pressedKey == SDLK_RIGHT)
                                Audio::playSfx("./assets/sfx/step.ogg");
                            else if (pressedKey == SDLK_a)
                                Audio::playSfx("./assets/sfx/bump.ogg");
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
                if (helpActive) {
                    SDL_RenderSetViewport(gRenderer, nullptr);
                    drawHelpScreen(gRenderer);
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