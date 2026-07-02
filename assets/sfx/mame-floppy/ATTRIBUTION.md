# 磁碟機音效來源

`assets/sfx/disk-mame.ogg`(F6 選「MAME」音源時,場景切換的 Apple II 5.25" 磁碟機讀取聲)
由本目錄的 MAME floppy sound 真實錄音樣本組合而成(起轉 + 跨道 seek + 找道 seek)。

> 磁碟機音效音源可在 F6 設定選單切換:關 / MAME(此檔)/ Applefritter(`disk-applefritter.ogg`,
> 另一組真實 Apple II 錄音)。新增音源只要放 `assets/sfx/disk-<音源>.ogg` 並加進程式的音源清單。

- **來源**:MAME 專案 `samples/floppy`(https://github.com/mamedev/mame/tree/master/samples/floppy)
- **授權**:CC0 1.0 Universal(公有領域,https://creativecommons.org/publicdomain/zero/1.0/)
  可商用、可修改、可散布,無需署名(此處署名為禮貌與完整性保全)。
- **檔案**:`525_*`=5.25" 磁碟機真實錄音(spin 馬達 / seek head 尋道 / step 單步)。
