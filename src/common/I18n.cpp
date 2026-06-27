#include "I18n.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdio>

static nlohmann::json gTable;

void I18n::init(const std::string &langPath) {
    std::ifstream in(langPath);
    if (!in.good()) {
        printf("[I18n] 語言檔不存在,回退 key: %s\n", langPath.c_str());
        return;
    }
    try {
        in >> gTable;
    } catch (const std::exception &ex) {
        printf("[I18n] 語言檔解析失敗: %s (%s)\n", langPath.c_str(), ex.what());
        gTable = nlohmann::json::object();
    }
}

std::string I18n::t(const std::string &key) {
    auto it = gTable.find(key);
    if (it != gTable.end() && it->is_string())
        return it->get<std::string>();
    return key;   // 缺 key → 回傳 key(便於發現漏譯)
}

std::string I18n::tf(const std::string &key, const std::vector<std::string> &args) {
    std::string s = t(key);
    for (size_t i = 0; i < args.size(); ++i) {
        std::string ph = "{" + std::to_string(i) + "}";
        size_t pos;
        while ((pos = s.find(ph)) != std::string::npos)
            s.replace(pos, ph.size(), args[i]);
    }
    return s;
}
