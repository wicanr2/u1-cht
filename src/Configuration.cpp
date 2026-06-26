#include "Configuration.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace std;

nlohmann::json Configuration::_value;
int Configuration::_speedPct = 100;
int Configuration::_spawnPct = 55;

void Configuration::init() {
    ifstream in("config.json");
    if (!in) {
        throw runtime_error("Could not open config.json");
    }
    in >> _value;

    // 初始化執行期可調值(F6 選單會改這兩個)
    _speedPct = clampPct(_value.value("speed_pct", 100));
    _spawnPct = clampPct(_value.value("spawn_pct", 55));

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
