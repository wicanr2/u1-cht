#include "Configuration.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace std;

nlohmann::json Configuration::_value;
int Configuration::_speedPct = 100;
int Configuration::_spawnPct = 55;
int Configuration::_foodPct = 100;
bool Configuration::_chaseMonsters = false;
string Configuration::_launchDir;
string Configuration::_resolvedGameFiles = "./gamedata/";

void Configuration::init() {
    // config.json 找:執行檔旁(chdir 後 CWD),再找啟動目錄(開發時在 repo 根、打包時在程式旁)
    ifstream in("config.json");
    if (!in && !_launchDir.empty()) in.open(_launchDir + "/config.json");
    if (!in) {
        throw runtime_error("Could not open config.json");
    }
    in >> _value;

    // 初始化執行期可調值(F6 選單會改這些)
    _speedPct = clampPct(_value.value("speed_pct", 100));
    _spawnPct = clampPct(_value.value("spawn_pct", 55));
    _foodPct = clampPct(_value.value("food_pct", 100));
    _chaseMonsters = _value.value("chase_monsters", false);

    // 解析版權 gamedata 目錄:先試執行檔旁(chdir 後的 CWD),找不到 MAP.BIN
    // 再試啟動目錄旁(AppImage/zip 把 BIN 放在程式旁的情形)。
    {
        string cfg = _value.value("game_files_path", string("./gamedata/"));
        auto hasMap = [](const string &dir) { ifstream f(dir + "MAP.BIN"); return f.good(); };
        string c2 = _launchDir.empty() ? cfg : (_launchDir + "/" + cfg);
        if (hasMap(cfg)) _resolvedGameFiles = cfg;
        else if (hasMap(c2)) _resolvedGameFiles = c2;
        else _resolvedGameFiles = cfg;   // 都找不到 → 保留;載入時報錯提示使用者放置 BIN
    }

    // validate that required configuration entries exist
    try {
        Configuration::getMapFilePath();
        Configuration::getScreenWidth();
        Configuration::getScreenHeight();
    } catch (const exception &ex) {
        auto msg = "Error trying to parse configuration file";
        cout << msg << ": " << ex.what() << endl;
        throw runtime_error(msg);
    }
}
