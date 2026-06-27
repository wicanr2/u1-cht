#include "Dungeon.h"
#include "DungeonFeature.h"
#include "enemies/DungeonMonster.h"
#include <cstdlib>

namespace {
using Shape = DungeonMonster::Shape;
// 地牢 bestiary 模板:名稱 key / 外形 / 顏色 / 基礎HP / 基礎傷害 / 最早出現層(0-based)
struct MobTpl { const char *key; Shape shape; Uint8 r, g, b; int hp, dmg, minLv; };
const MobTpl kBestiary[] = {
    {"monster.giant_rat",       Shape::Beast,    0xB0, 0x80, 0x50,  5, 1, 0},
    {"monster.giant_spider",    Shape::Beast,    0xA0, 0x60, 0xC0,  8, 2, 0},
    {"monster.skeleton",        Shape::Humanoid, 0xE0, 0xE0, 0xE0, 10, 2, 0},
    {"monster.gremlin",         Shape::Beast,    0x60, 0xC0, 0x60,  7, 2, 1},
    {"monster.carrion_creeper", Shape::Beast,    0x80, 0xA0, 0x40, 16, 3, 2},
    {"monster.gelatinous_cube", Shape::Blob,     0x40, 0xC0, 0xA0, 20, 3, 2},
    {"monster.cyclops",         Shape::Humanoid, 0xC0, 0x90, 0x50, 24, 4, 3},
    {"monster.mind_whipper",    Shape::Flyer,    0xC0, 0x40, 0xC0, 22, 4, 3},
    {"monster.invisible_seeker",Shape::Humanoid, 0x60, 0x60, 0x80, 26, 5, 4},
    {"monster.lich",            Shape::Humanoid, 0xC0, 0xF0, 0xC0, 30, 5, 5},
    {"monster.daemon",          Shape::Flyer,    0xF0, 0x50, 0x30, 34, 6, 5},
    {"monster.zorn",            Shape::Blob,     0xF0, 0xC0, 0x40, 30, 6, 6},
    {"monster.balron",          Shape::Flyer,    0xFF, 0x30, 0x30, 44, 8, 6},
};
constexpr int kBestiaryN = sizeof(kBestiary) / sizeof(kBestiary[0]);
}

// 生成一層:偶數欄=開放走廊(None),奇數欄=牆+2~3 隨機門(保證可橫越)。9×9。
static vector<vector<DungeonFeature>> buildLevel() {
    constexpr int N = 9;
    vector<vector<DungeonFeature>> cols(N, vector<DungeonFeature>(N, DungeonFeature::None));
    for (int c = 1; c < N; c += 2) {
        for (int y = 0; y < N; y++) cols[c][y] = DungeonFeature::Wall;
        int doors = 2 + rand() % 2;   // 2~3 道門
        for (int d = 0; d < doors; d++) cols[c][1 + rand() % (N - 2)] = DungeonFeature::Door;
    }
    return cols;
}

void Dungeon::randomize() {
    constexpr int LEVELS = 8;
    // 開放格(偶數欄)隨機座標,避開入口(2,0)與兩個梯位(2,4)(6,4)
    auto randOpenCell = [&](int &ox, int &oy) {
        for (int t = 0; t < 30; t++) {
            int x = (rand() % 5) * 2;       // 0,2,4,6,8
            int y = 1 + rand() % 7;
            if ((x == 2 && y == 0) || (x == 2 && y == 4) || (x == 6 && y == 4)) continue;
            ox = x; oy = y; return;
        }
        ox = 4; oy = 2;
    };

    for (int lv = 0; lv < LEVELS; lv++) {
        _levels.push_back(buildLevel());

        // 梯:L0 入口(2,0)上梯→出地牢;各層 (6,4) 下梯→下層、(2,4) 上梯→上層
        vector<LadderInfo> ladders;
        if (lv == 0) ladders.emplace_back(2, 0, 2, 0, true);          // 出地牢
        else         ladders.emplace_back(2, 4, 6, 4, true);          // → 上層 (6,4)
        if (lv < LEVELS - 1) ladders.emplace_back(6, 4, 2, 4, false); // → 下層 (2,4)
        _laddersPerLevel.push_back(ladders);

        // 怪:數量隨深度 1 + lv/2(最多 5);只挑 minLv<=lv 的模板,HP/傷害隨深度微升
        vector<shared_ptr<Enemy>> mobs;
        int count = 1 + lv / 2;
        if (count > 5) count = 5;
        for (int m = 0; m < count; m++) {
            // 收集合格模板
            int pool[kBestiaryN], n = 0;
            for (int i = 0; i < kBestiaryN; i++) if (kBestiary[i].minLv <= lv) pool[n++] = i;
            const MobTpl &t = kBestiary[pool[rand() % n]];
            int ex, ey; randOpenCell(ex, ey);
            int hp = t.hp + lv;                 // 深度微強化
            int dmg = t.dmg + lv / 3;
            mobs.push_back(make_shared<DungeonMonster>(ex, ey, hp, dmg, t.key, t.shape, t.r, t.g, t.b));
        }
        _enemiesPerLevel.push_back(mobs);

        // 寶箱:每層 1 個,金幣隨深度,約半數上鎖(需開鎖術/開棺術或踩中陷阱)
        vector<shared_ptr<ChestInfo>> chests;
        int cx, cy; randOpenCell(cx, cy);
        int gold = 30 + lv * 20 + rand() % 40;
        bool locked = (rand() % 2 == 0);
        chests.push_back(make_shared<ChestInfo>(cx, cy, gold, locked));
        _chestsPerLevel.push_back(chests);
    }

    if (getenv("U1_DUMP_DUNGEON")) {
        for (int lv = 0; lv < (int)_enemiesPerLevel.size(); lv++) {
            printf("DUNGEONLV %d enemies:", lv);
            for (auto &e : _enemiesPerLevel[lv]) printf(" %s@%d,%d", e->getName().c_str(), e->getX(), e->getY());
            printf(" | chests:");
            for (auto &c : _chestsPerLevel[lv]) printf(" %dG@%d,%d%s", c->gold, c->x, c->y, c->locked ? "(locked)" : "");
            printf(" | ladders:");
            for (auto &l : _laddersPerLevel[lv]) printf(" %s@%d,%d->%d,%d", l.goingUp ? "up" : "down", l.fromX, l.fromY, l.toX, l.toY);
            printf("\n");
        }
        fflush(stdout);
    }
}

DungeonFeature Dungeon::lookLeftFromEast(vector<vector<DungeonFeature>> levelFeatures, int x, int y) {
    return y == 0 ? DungeonFeature::Wall : levelFeatures[x][y - 1];
}

DungeonFeature Dungeon::lookRightFromEast(vector<vector<DungeonFeature>> levelFeatures, int x, int y) {
    return y == SIZE - 1 ? DungeonFeature::Wall : levelFeatures[x][y + 1];
}

DungeonFeature Dungeon::lookLeftFromWest(vector<vector<DungeonFeature>> levelFeatures, int x, int y) {
    return y == SIZE - 1 ? DungeonFeature::Wall : levelFeatures[x][y + 1];
}

DungeonFeature Dungeon::lookRightFromWest(vector<vector<DungeonFeature>> levelFeatures, int x, int y) {
    return y == 0 ? DungeonFeature::Wall : levelFeatures[x][y - 1];
}

DungeonFeature Dungeon::lookLeftFromNorth(vector<vector<DungeonFeature>> levelFeatures, int x, int y) {
    return x == 0 ? DungeonFeature::Wall : levelFeatures[x - 1][y];
}

DungeonFeature Dungeon::lookRightFromNorth(vector<vector<DungeonFeature>> levelFeatures, int x, int y) {
    return x == SIZE - 1 ? DungeonFeature::Wall : levelFeatures[x + 1][y];
}

DungeonFeature Dungeon::lookLeftFromSouth(vector<vector<DungeonFeature>> levelFeatures, int x, int y) {
    return x == SIZE - 1 ? DungeonFeature::Wall : levelFeatures[x + 1][y];
}

DungeonFeature Dungeon::lookRightFromSouth(vector<vector<DungeonFeature>> levelFeatures, int x, int y) {
    return x == 0 ? DungeonFeature::Wall : levelFeatures[x - 1][y];
}

DungeonFeature Dungeon::lookLeft(vector<vector<DungeonFeature>> levelFeatures, int x, int y, CardinalPoint point) {
    switch (point) {
        case CardinalPoint::East:
            return lookLeftFromEast(levelFeatures, x, y);
        case CardinalPoint::West:
            return lookLeftFromWest(levelFeatures, x, y);
        case CardinalPoint::North:
            return lookLeftFromNorth(levelFeatures, x, y);
        case CardinalPoint::South:
            return lookLeftFromSouth(levelFeatures, x, y);
    }
}

DungeonFeature Dungeon::lookRight(vector<vector<DungeonFeature>> levelFeatures, int x, int y, CardinalPoint point) {
    switch (point) {
        case CardinalPoint::East:
            return lookRightFromEast(levelFeatures, x, y);
        case CardinalPoint::West:
            return lookRightFromWest(levelFeatures, x, y);
        case CardinalPoint::North:
            return lookRightFromNorth(levelFeatures, x, y);
        case CardinalPoint::South:
            return lookRightFromSouth(levelFeatures, x, y);
    }
}

vector<VisibleDungeonSpace> Dungeon::getVisible(int level, int x, int y, CardinalPoint viewDirection) {
    auto levelFeatures = _levels[level];
    vector<VisibleDungeonSpace> visibleSpaces;
    int i = 1;
    bool wall = false;

    // Get the space we're in
    auto dungeonSpace = VisibleDungeonSpace(levelFeatures[x][y],
                                            lookLeft(levelFeatures, x, y, viewDirection),
                                            lookRight(levelFeatures, x, y, viewDirection));

    // See if there is a ladder where we're standing.
    // I don't like that I'm doing this check again later for other tiles.
    for (const auto &ladder : _laddersPerLevel[level]) {
        if (ladder.fromX == x && ladder.fromY == y) {
            dungeonSpace.ladder = make_shared<LadderInfo>(ladder);
        }
    }
    dungeonSpace.chest = getChestAt(level, x, y);

    visibleSpaces.push_back(dungeonSpace);

    while (i < MAX_VISIBILITY && !wall) {
        int visibleTileX;
        int visibleTileY;
        bool outOfBounds = false;
        switch (viewDirection) {
            case CardinalPoint::East: {
                visibleTileX = x + i;
                visibleTileY = y;

                outOfBounds = visibleTileX == SIZE;
            }
                break;
            case CardinalPoint::West: {
                visibleTileX = x - i;
                visibleTileY = y;

                outOfBounds = visibleTileX == -1;
            }
                break;
            case CardinalPoint::North: {
                visibleTileX = x;
                visibleTileY = y - i;

                outOfBounds = visibleTileY == -1;
            }
                break;
            case CardinalPoint::South: {
                visibleTileX = x;
                visibleTileY = y + i;

                outOfBounds = visibleTileY == SIZE;
            }
                break;
        }

        auto cellFeature = outOfBounds ? DungeonFeature::Wall : levelFeatures[visibleTileX][visibleTileY];
        if (isWalledFeature(cellFeature)) wall = true;

        auto leftFeature = wall ? DungeonFeature::Wall : lookLeft(levelFeatures, visibleTileX, visibleTileY,
                                                                  viewDirection);
        auto rightFeature = wall ? DungeonFeature::Wall : lookRight(levelFeatures, visibleTileX, visibleTileY,
                                                                    viewDirection);
        dungeonSpace = VisibleDungeonSpace(cellFeature,
                                           leftFeature,
                                           rightFeature);

        if (level < _enemiesPerLevel.size()) {
            for (const auto &enemy : _enemiesPerLevel[level]) {
                if (enemy->getX() == visibleTileX && enemy->getY() == visibleTileY && !enemy->isDead()) {
                    dungeonSpace.enemy = enemy;
                    break;
                }
            }
        }

        if (level < _laddersPerLevel.size()) {
            for (const auto &ladder : _laddersPerLevel[level]) {
                if (ladder.fromX == visibleTileX && ladder.fromY == visibleTileY) {
                    dungeonSpace.ladder = make_shared<LadderInfo>(ladder);
                    break;
                }
            }
        }
        if (!wall) dungeonSpace.chest = getChestAt(level, visibleTileX, visibleTileY);

        visibleSpaces.push_back(dungeonSpace);
        i++;
    }

    return visibleSpaces;
}
