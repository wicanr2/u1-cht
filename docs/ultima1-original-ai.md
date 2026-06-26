# 原版 Ultima I 怪物 AI 考據(整合)

> 給 open_ultima 實作怪物行為的權威依據。
> **每條標出處等級**:`[CODE]`=1983 Atari 原碼反組譯確認;`[BEHAV]`=多來源行為描述一致;`[ARCH]`=RE 架構推得。
> 過程紀錄見 `docs/re/`;6502 判讀經驗見 `docs/re/6502-re-methodology.md`。考據:2026-06-26。

## 1. 核心結論(可直接拿來實作)

| 場景 | 怪物行為 | 出處 |
|---|---|---|
| **地面 overworld** | **不移動**;以隨機遭遇「出現在玩家位置即攻擊」 | `[BEHAV]` 多來源一致 + `[ARCH]` |
| **地牢 dungeon** | **逐玩家移動(beeline)**,進攻擊範圍持續攻擊;**怪可對角攻擊,玩家不行** | `[BEHAV]` 多來源一致 |
| 戰鬥節奏 | 回合制,**有行動時限**(沒在時限內動就喪失該回合);承襲 Akalabeth | `[BEHAV]` |

> 一句話:**U1「會動的怪」只在地牢;地面怪不動(出現即打)。** 地牢怪是貪婪逐玩家 + 貼身/對角攻擊,
> 無路徑規劃。⚠️ **不可套 u2-cht 的 U2 追擊 AI**(那是另一作品反組譯)。

## 2. RE 確認的引擎架構(`[CODE]` / `[ARCH]`,1983 Atari SierraVenture)

從 Side A 反組譯確認的事實(支撐上面結論、也是日後續追的地圖):

- **鏈載入器** `[CODE]`:`ULTIMA` 檔內嵌字串 `D:INITLOAD`,逐檔鏈載。
- **情境式 MOVE overlay** `[CODE]`:`OUTMOVE`/`DNGMOVE`/`TWNMOVE`/`CASMOVE`/`SPAMOVE` ——
  各情境一個檔,載到 `$6A00`+`$8000`;**內容大半是資料**(地牢 3D 線框頂點 + graphics mask),
  程式碼只有少量 helper。
- **地牢牆 = 1-bit/cell 點陣圖** `[CODE]`:`$6A00`/`$6C28` 是「座標→byte 位址 + bit」存取器,
  用單位元遮罩表 `$6C6E = 80 40 20 10 08 04 02 01`;牆/通道用一個 bit 表示。
- **常駐引擎 + callback 註冊** `[CODE]`:overlay 用 `$6C45`(`sta $3019/$301A`)把自己的資料指標
  寫進常駐引擎($3000 區)的向量;**通用回合迴圈(含怪物移動)在常駐引擎,各情境共用**。
  → 這解釋為何「地牢怪會動、地面怪不動」可由**同一套移動碼 + 不同情境旗標**達成。

## 3. 尚未由原碼隔離的部分(誠實揭露)

- **怪物移動的確切 6502 演算法(逐步軸序、是否含亂數、攻擊命中公式)尚未從原碼逐行確認。**
  原因:移動邏輯在**常駐引擎**,而常駐引擎由鏈載入器多檔組成 + 疑似從 $8000 暫存區**重定位**到 $3000,
  且 overlay 無獨立入口點(入口在常駐調度表)。要逐行確認 = 反組譯整個常駐引擎(多日工程)。
  **這是真實架構牆,非略過**;續追路徑見 `docs/re/README.md`(抽常駐引擎 → $3000 調度 → 回合迴圈)。
- 因此上面「地牢 beeline + 對角攻擊」是 `[BEHAV]`(行為多來源一致),架構 `[ARCH]` 支持其合理,
  但**逐行 6502 尚待 `[CODE]` 補強**。

## 4. open_ultima 實作對照(忠於原版)

| 場景 | 原版 | open_ultima 現況 | 忠於原版補法 |
|---|---|---|---|
| 地面 | 怪不移動,出現即攻擊 | 怪生成不還手(已補相鄰反擊,見 d6edb74) | ✅ 維持「不移動、相鄰才攻擊」 |
| 地牢 | 怪逐玩家移動 + 對角攻擊 | 怪有座標但**無移動**(靜止) | 加「每回合 beeline 逐玩家一格 + 8 向相鄰攻擊」 |

實作要點(依架構,beeline 貪婪逐近):
- 每回合每隻地牢怪:若與玩家相鄰(Chebyshev=1,含對角)→ 攻擊;否則朝玩家走一格
  (取 X/Y 距離較大軸前進,目標格非牆且未被佔)。
- 對角攻擊是 U1 特色(玩家只能正面攻擊)。
- 牆判定對應原版的 1-bit/cell 點陣圖(open_ultima 用 `DungeonFeature` per cell,語意等價)。

## 5. 來源

行為 `[BEHAV]`:
- [Ultima I monster data — Codex of Ultima Wisdom](https://wiki.ultimacodex.com/wiki/Ultima_I_monster_data)
- [The CRPG Addict — Game 4: Ultima (1981)](http://crpgaddict.blogspot.com/2010/02/game-4-ultima-i.html)
- [Dino's Complete Guide to Ultima 1](https://gigi.nullneuron.net/ultima/u1/walkthrough.php)
- [Tips & Tricks — Ultima I (gamercorner)](https://guides.gamercorner.net/ultimai/tips-and-tricks/)

原碼 `[CODE]`:1983 Atari SierraVenture(`org_game/Ultima I …Atari…`),反組譯流程見 `docs/re/`。
