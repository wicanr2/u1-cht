#pragma once

#include <string>
#include <vector>

// i18n 查表層:把使用者可見字串外移到 assets/strings/<lang>.json(key → 譯文),
// 程式以 key 取字。缺 key / 未載入 → 回傳 key 本身(不崩潰、便於發現漏譯)。
// 插值用 {0} {1} … 佔位符,以 tf(key, {arg0, arg1, …}) 代入。
class I18n {
public:
    // 載入語言檔(預設 assets/strings/zh-Hant.json)。失敗則之後 t() 回傳 key。
    static void init(const std::string &langPath);

    // 取譯文(缺 key 回傳 key)。
    static std::string t(const std::string &key);

    // 取譯文並代入 {0}{1}… 佔位符。
    static std::string tf(const std::string &key, const std::vector<std::string> &args);
};
