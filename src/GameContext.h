#pragma once

#include <memory>
#include <utility>
#include "Player.h"
#include "ScreenType.h"
#include "Configuration.h"
#include "common/Audio.h"

using namespace std;

class GameContext {
public:
    explicit GameContext(shared_ptr<Player> player) : _player(std::move(player)) {}

    void setScreen(ScreenType screenType) {
        if (screenType != _currentScreen) _onSceneChange();
        _currentScreen = screenType;
    }

    void enterTown() {
        _onSceneChange();
        _currentScreen = ScreenType::Town;
        _inCastle = false;
    }

    void enterCastle() {
        _onSceneChange();
        _currentScreen = ScreenType::Town;
        _inCastle = true;
    }

    ScreenType getCurrentScreen() { return _currentScreen; }

    shared_ptr<Player> getPlayer() { return _player; }

    bool isInCastle() { return _inCastle; }

private:
    // 場景切換 hook:開啟磁碟機音效時,每次進出場景播一次 Apple II 5.25" 讀取聲(issue #2)。
    // 缺檔時 Audio::playSfx 靜默忽略,不影響遊戲。
    void _onSceneChange() {
        if (Configuration::getDiskSound()) Audio::playSfx("./assets/sfx/disk.ogg");
    }

    ScreenType _currentScreen = ScreenType::Overworld;
    shared_ptr<Player> _player;
    bool _inCastle = false;
};

