#include "Configuration.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace std;

nlohmann::json Configuration::_value;

void Configuration::init() {
    ifstream in("config.json");
    if (!in) {
        throw runtime_error("Could not open config.json");
    }
    in >> _value;

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
