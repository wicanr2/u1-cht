# 素材來源清單(原始遊戲 / 參考)

> 版權素材一律**不入庫**(`.gitignore` 排除 `org_game/`、`reference/`、`gamedata/`、`*.BIN`)。
> 本檔只記「放哪、什麼格式、拿來做什麼」。對應 `docs/adr/0001-pluggable-asset-packs.md`。

## 1. open_ultima 直接使用(DOS 版,已接通)
| 路徑 | 內容 | 狀態 |
|---|---|---|
| `org_game/002552_ultima_1_ibm_pc.7z` → `disk1.img` | IBM PC 版 360KB 軟碟 | ✅ 已解出 |
| `gamedata/*.BIN` | 由 disk1.img 取出的 EGA/CGA/Tandy tiles、MAP、TCD 等 | ✅ Phase 1 實跑讀取成功 |

DOS 版內含三套 tileset(同類 BIN 格式,可直接切):
`EGATILES.BIN`(EGA)、`CGATILES.BIN`(CGA)、`T1KTILES.BIN`(Tandy 1000)。

## 2. 其他平台版本(素材包來源,待拆 sprite — ADR 0001)
| 平台 | 路徑 | 格式 |
|---|---|---|
| Apple IIgs | `org_game/Ultima I IIgs (woz-a-day collection).zip` (+extras) | woz 磁碟映像 |
| Atari 8-bit | `org_game/Ultima I - AllFiles-atari.zip` | Atari 檔 |
| MSX | `org_game/msx/Ultima I ...(1986)(Pony Canyon)(ja).zip` → `.dsk` | MSX 磁碟映像(日版) |
| PC-98 | `org_game/msx/【PC98】Ｕｌｔｉｍａ.rar` → `.fdi` | PC-98 軟碟映像(本體 + Save) |
| Trilogy (DOS) | `org_game/Ultima_Trilogy_1989.iso` | 合輯 ISO |
| **FM Towns** | `../u7-cht/Ultima-Trilogy-I-II-III_FM-Towns_JA-EN_UserDisk-incl.zip` | Trilogy I/II/III CD 映像(`.cue/.img` ~141MB + UserDisk `.hdm`);**U1 FM Towns 美術在此合集內** |

## 3. 參考資料
| 用途 | 路徑 |
|---|---|
| 各版本截圖(9 平台 42 張,拆 sprite 比對) | `reference/hg101/imgs/` + `INDEX.md`(來源:hardcoregaming101) |
| 官方手冊 PDF | `org_game/Ultima_1_The_First_Age_of_Darkness.pdf` |
| FM Towns 抽圖 / tileset 流程參考 | `../u7-cht/u2-cht/`(創世紀 2 中文化:`tileset/`、`oracle/`、`tools/`) |

## 4. 待辦(素材相關)
- [ ] MSX `.dsk` / PC-98 `.fdi` 內美術格式解析(各平台不同,逐版評估)。
- [ ] 統一 PNG sprite sheet 切版規格(tile 尺寸 / 索引 / 遮罩)。
- [ ] 參考 u2-cht 的 FM Towns 抽圖工具鏈,評估可複用部分。
