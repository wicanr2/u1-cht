# 打包與交付工程筆記

這份筆記記錄創世紀一代中文化在「打包成跨平台成品、交付給玩家」這段路上踩到的幾個真實問題與解法。對象是之後要做類似老遊戲移植 / 中文化的人——這些坑跟遊戲內容無關,而是「把一份能在開發機跑的程式,變成別人雙擊就能玩的東西」時共通會遇到的。

---

## 一、打包後「資源全找不到」——工作目錄陷阱

**現象**:macOS 玩家雙擊 `.app`,中文顯示不出來、圖磚與音樂載入失敗,log 一串 `No such file or directory`;但原版遊戲資料 `gamedata/*.BIN` 卻讀得到。(來自 issue #1 的實際回報)

**根因**:程式用相對路徑找資源,例如 `assets/tilesets/fmtowns.png`、`assets/fonts/u1-cjk.ttf`。相對路徑是相對「工作目錄(cwd)」解析的,而 macOS 用 Finder 雙擊 `.app` 時,cwd 是 `/`(根目錄)——不是執行檔所在目錄。於是所有 `assets/...` 全部找不到,字型載不到又進一步導致 `SDL_ttf: Passed a NULL pointer`、中文變空白。

那為什麼 `gamedata` 沒事?因為它另有一套「找不到就退回啟動目錄再找一次」的後備搜尋,`assets` 卻沒有——**同一支程式裡兩套資源用了兩種待遇,只有一套扛得住 cwd 改變**。這種不對稱最容易漏,因為開發時從 repo 根目錄跑,cwd 剛好對,兩套都能過,測不出來。

**解法**(`main.cpp`,啟動最前面):

```cpp
// 先記原始 cwd(供 gamedata 後備搜尋:AppImage 旁、使用者放 BIN 的位置)
char launch[4096];
if (getcwd(launch, sizeof(launch))) Configuration::setLaunchDir(launch);
// 再把 cwd 切到執行檔所在,讓所有相對路徑 assets/... 一律對齊
char *base = SDL_GetBasePath();
if (base) { chdir(base); SDL_free(base); }
```

**通則**:任何「解壓即跑」的打包產物(macOS `.app`、Linux AppImage、Windows 綠色版 zip),**啟動第一件事就是 `chdir` 到執行檔目錄**。順序很重要——先記原始 cwd(有些資源如使用者自備的版權檔會放在啟動目錄旁),再切換。

> 這類 bug headless 測不到(dummy 環境下 cwd 剛好對、或根本進不到載入),一定要驗**打包產物本身**在它自己的啟動情境下跑(雙擊、或從別的目錄啟動)。

---

## 二、程式碼、自製資產、版權原版——三層分離

老遊戲中文化的產物混了三種東西,授權天差地別,打包前要分清楚:

| 層 | 例子 | 授權 | 能不能公開 |
|---|---|---|---|
| **程式碼** | 引擎、遊戲邏輯、SDL 移植層 | 自寫(可開源) | ✅ 公開 GitHub |
| **自製資產** | 中文字型子集、UI、自繪圖 | 自製 | ✅ 可公開 |
| **版權原版** | 原版遊戲資料 `*.BIN`、平台原版 BGM | 原廠版權 | ❌ 不入庫、不散布 |

對應到這個 repo 的實際做法:

- 版權原版 BGM 用 `.gitignore` 擋在版控外(`assets/music/*.ogg`,只留一首自製占位曲 `theme.ogg`)。
- 原版 `*.BIN` 同樣不入庫,由玩家自備、或本機做「個人完整包」時才注入。
- 交付因此分兩種:**公開版**只有程式碼;**個人完整包**是本機把版權資料注入後自留,不上網。

---

## 三、缺了版權素材,要優雅降級而不是噴錯

版權 BGM 被 gitignore 擋掉後,公開建置的產物自然缺 `fmtowns.ogg` 這類檔。舊版程式碼每次要播就噴一行 `Mix_LoadMUS failed (...)`,場景一切換噴一串,讓玩家以為程式壞了。

**解法**:把「版權素材缺失」當成預期路徑,不是錯誤——

```cpp
// 平台原版 BGM 常未隨附:quiet 嘗試,缺檔靜默改用占位曲,不噴 error
bool quiet = (m != kDefaultMusic);
if (!Audio::playMusic(m, quiet) && m != kDefaultMusic) Audio::playMusic(kDefaultMusic);
```

只有連占位曲都載不到(真的有問題)才印診斷。**預期會缺的東西,缺了要安靜;不該缺的東西缺了,才吵。**

---

## 四、音效素材:用真實錄音,別自己合成

retro 作品的聲音玩家有記憶,自己用合成器「逼近」一聽就假。原則是**用真實素材**。

issue #2 要「場景切換播 Apple II 5.25" 磁碟機讀取聲」,素材來源最後用 **MAME 專案的 `samples/floppy`**:

- 它是真實磁碟機錄音,而且授權是 **CC0 1.0**(公有領域,可商用、可修改、免署名)——對任何專案都乾淨。
- 檔名有規律:`35_*` 是 3.5" 磁碟機、`525_*` 是 5.25"(Apple II Disk II 用這組),各含 spin(馬達)/ seek(尋道)/ step(單步)。
- 這個專案用 `525_spin_start_loaded + 525_seek_20ms + 525_seek_6ms` 串成一段 0.7 秒的「起轉 → 跨道 → 找道」讀取聲(`assets/sfx/disk.ogg`),原始樣本與授權說明保留在 `assets/sfx/mame-floppy/`。

下載點:<https://github.com/mamedev/mame/tree/master/samples/floppy>(授權見該目錄 `LICENSE`)。

---

## 五、發布邊界:GitHub 只公開程式碼

這個專案刻意**不在 GitHub 發布任何編譯好的執行檔**(連不含版權資料的精簡 zip 也不發),原因見第二節——編譯後的二進位捆綁了美術/字型/音樂等資產,授權比純程式碼複雜。界線劃在「原始碼公開、任何 build 產物都不對外」最乾淨。

落實在 CI(`.github/workflows/release.yml`):

- 移除了自動建 GitHub Release 的步驟,`permissions: contents: read`。
- 四平台建置產物只上傳到 **Actions artifact**(供維護者本機 `gh run download` 抓回,做個人完整包)。
- 要標版本就打 `git tag`——GitHub 自動附的「Source code」是純原始碼壓縮檔,不含 build 產物,安全。

> 玩家要成品:由維護者本機建置後私下提供,或玩家自行 clone 建置。不透過公開 Release 散布二進位。

---

## 六、macOS 自編 SDL 的 dylibbundler 陷阱

macOS 這邊還踩過一個把 CI 卡到逾時(40 分鐘)的坑:自編(from-source)的 SDL dylib,install name 是 `@rpath/...`,`dylibbundler` 解不到實體會**進互動模式問路徑**,CI 沒有 stdin 就無限等待。修法是給它搜尋路徑 `-s "$PREFIX/lib"` 並把 stdin 導向 `/dev/null` 讓它 fail-fast。

這條連同「自編 SDL 砍掉沒用到的 codec 加速編譯」「CI 長時間卡住怎麼用時間戳逐步定位」等完整經驗,整理在跨專案的 `mac-app-cross-pack` skill,這裡不重複。

---

## 一句話總結

老遊戲中文化的難點,後半段不在「翻譯」而在「交付」:**讓程式在別人的機器上、用別人的啟動方式、在缺了版權素材的情況下,依然乾淨地跑起來**——同時把該公開的(程式碼)和不該散布的(版權資料)分得清清楚楚。
