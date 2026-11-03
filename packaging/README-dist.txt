創世紀一代（Ultima I: The First Age of Darkness）繁體中文 remake
================================================================

【放置原版遊戲資料（必要）】
本程式不含版權遊戲資料。請自備 1981/1986 原版 Ultima I 的資料檔，
把這些 *.BIN 放進程式旁的 gamedata/ 資料夾：
  MAP.BIN  EGATILES.BIN  CGATILES.BIN  EGATOWN.BIN  CGATOWN.BIN  TCD.BIN
（AppImage 版：把 gamedata/ 放在 .AppImage 檔案旁邊即可。）

【執行】
  Windows : 雙擊 u1_cht.exe
  macOS   : 開啟 Ultima1-CHT.app（或 ./u1_cht）
  Linux   : ./Ultima1-CHT.AppImage  或  ./u1_cht

【存檔】
存檔自動寫入使用者目錄（Windows: %APPDATA%\LCY\Ultima1-CHT、
macOS: ~/Library/Application Support/、Linux: ~/.local/share/）。

【操作】遊戲內按 F1 看完整指令說明。

【這個版本的內容】
這是精簡版，不含各平台的美術圖塊，也不含平台原版音樂。
遊戲內按 PageDown 可切換 EGA、CGA，兩者都讀取你自備的 BIN。
FM Towns、MSX、PC-98、Apple IIgs、Atari 的畫面要從原始碼建置才有。

【授權】
程式碼採 GPL-3.0（繼承 open_ultima），條款見 LICENSE.txt。
原始碼：https://github.com/wicanr2/u1-cht
中文字型 u1-cjk.ttf 是 Noto Serif CJK 的子集，條款見 LICENSE-NotoSerifCJK.txt。
磁碟機音效來自 MAME 專案，出處見 assets/sfx/mame-floppy/ATTRIBUTION.md。
原版遊戲資料屬原權利人，不包含在內。
