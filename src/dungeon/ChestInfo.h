#pragma once

// 地牢寶箱:位置 + 金幣 + 是否上鎖(需開鎖術/開棺術)+ 是否已開。
struct ChestInfo {
    int x, y, gold;
    bool locked;
    bool opened = false;
    ChestInfo(int x_, int y_, int gold_, bool locked_) : x(x_), y(y_), gold(gold_), locked(locked_) {}
};
