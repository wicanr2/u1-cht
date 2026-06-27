# 規劃:城鎮內容 + 戰鬥深度

> open_ultima 上游把世界地圖/地牢/地面戰鬥做了骨架,但**城鎮是空殼**(只有地圖+移動)、
> **戰鬥無深度**(固定傷害、無裝備/法術/屬性)。本規劃確認資料三要件齊備後,提出分階段實作。

## 0. 資料三要件確認 ✅

| 要件 | 狀態 | 內容 |
|---|---|---|
| **手冊** | ✅ | `org_game/Ultima_1_The_First_Age_of_Darkness.pdf`(21 頁,word-for-word 原版手冊)。已讀:城鎮/城堡、商店(防具/武器/魔法/食物/酒館)、角色(4 種族 ×4 職業 + 6 屬性)、魔法(9 法術)、戰鬥、完整 bestiary(37 種怪)。中文聖者之書另見 `../u4-cht/manuel/`。 |
| **素材** | ✅ | `gamedata/TCD.BIN`(城鎮/城堡地圖,**已被 `TownManager` 載入**)、`EGATOWN.BIN`(城鎮 tile)、`EGAFIGHT/EGAMOND.BIN`(戰鬥/怪物圖)、`ULTIMA.EXE`(36KB,原版含商店價格/任務/NPC 對話)。 |
| **RE 經驗** | ✅ | `docs/re/6502-re-methodology.md`(遞迴下降反組譯方法論,可換 x86 套用)、已逆向多平台格式的工具鏈。`ULTIMA.EXE` 可反組譯取精確數值;但**商店品項/價格/法術/任務在手冊與 Ultima Codex wiki 已完整公開,可直接 author**(小量資料、更快),RE 當校對後備。 |

## 1. 現況盤點(open_ultima 已有 / 缺口)

**已有**
- `TownManager`:讀 `TCD.BIN`(51 tile 類型、多張城鎮圖,top→bottom/left→right grid)→ `Town`/`TownTile`。
- `TownScreen`:城鎮地圖渲染 + 方向鍵移動 + `E` 進出。
- `Player`:`_hp`(150)/`_food`(200)/`_xp`(0)/`_money`(100)。地面/地牢相鄰攻擊(固定 2–6 傷害)。

**缺口**
- 城鎮:**無商店互動**(踩到武器/防具/食物/魔法店 tile 不會買賣)、無酒館線索、無守衛(偷竊→攻擊)、無 NPC 走動。
- 城堡:無國王(給任務 / 加屬性)。
- 戰鬥深度:Player **無武器/防具/法術/物品欄/屬性**;傷害固定不看裝備;XP 不升級;死亡無處理。

## 2. U1 機制(手冊 + wiki 摘要,作為實作規格)

- **角色屬性**(6):力量 Strength、敏捷 Agility、體力 Stamina、魅力 Charisma、智慧 Wisdom、智力 Intelligence。+ HP、Food、Gold、XP。
- **武器**(由弱到強,16 階):手 / 匕首 / 弓 / 錘 / 斧 / 劍 / 大劍 / …(後段含 phaser/blaster 科幻武器)。傷害隨階。
- **防具**(8 階):無 / 皮甲 / 鏈甲 / 板甲 / …(含 reflect/power 等)。減傷隨階。
- **商店**(城鎮/城堡內,踩 tile 觸發):
  - 武器店 / 防具店:買裝備(花 gold)。
  - 食物店:買食物(回 food)。
  - 魔法店:買法術(花 gold + xp)。
  - 酒館 Pub:花錢買線索/八卦(推進主線)。
  - 馬廄 / 交通:買馬/船等載具。
- **魔法**(9):Blink、Create、Destroy、Kill、Ladder Down/Up、Magic Missile、Open、Prayer、Unlock。
- **國王(城堡)**:給任務(殺特定怪/到地牢深層)→ 獎勵 gold/屬性點。
- **戰鬥**:傷害 = f(武器階, 力量) − f(目標防禦);受傷 = f(怪攻擊, 我方防具)。XP 累積 → 屬性/HP 成長。死亡 → 回 Lord British 城堡復活(扣資源)。

## 3. 資料結構設計(新增)

```
Player 擴充:
  屬性 strength/agility/stamina/charisma/wisdom/intelligence
  裝備 currentWeapon, currentArmor
  物品欄 weapons[16](擁有數量), armor[8], spells[9](擁有數量)
  vehicle(none/horse/cart/raft/frigate/...)

assets/data/shops.json   —— 各店品項 + 價格(author 自手冊/wiki,可 i18n)
assets/data/towns.json   —— 各城鎮/城堡的 shop tile → 店家類型對應、NPC、國王任務
assets/strings/zh-Hant.json 既有 i18n 層 —— 商店/對話/任務文字走查表
```

> 商店/任務文字一律走既有 **i18n 查表層**(`I18n::t/tf`),不內聯。

## 4. 分階段實作(一個一個做,每階段可玩可驗證)

**Phase A — 商店系統(城鎮核心)**
1. `Player` 加屬性 + 裝備 + 物品欄 + 對應 getter/setter,存檔序列化(`SaveGame` 擴充)。
2. `TownScreen`:踩到 shop tile(由 `towns.json` 對應)→ 開**商店 modal**(自繪中文,沿用 F6/離開鐵則的 modal 風格):列品項+價格、↑↓選、Enter 買、ESC 離開。買賣改 gold/food/裝備/法術。
3. 食物店、武器店、防具店、魔法店先到位。`PlayerStatusDisplay` 顯示目前武器/防具(可選)。
4. 驗證:進城鎮 → 買劍/皮甲/食物 → 狀態變化 → game tester 截圖。

**Phase B — 戰鬥深度**
5. 傷害公式接裝備:`OverworldScreen`/`DungeonScreen` 攻擊傷害 = f(武器, 力量);受傷 = f(怪, 防具)。
6. XP → 升級(HP/屬性成長)、死亡 → 回 Lord British 城堡復活(扣資源,接存檔)。
7. 驗證:換武器前後傷害差異、升級、死亡復活。

**Phase C — 城堡 / 國王 / 法術 / 酒館**
8. 城堡國王:對話 modal 給任務 + 領獎(加屬性/gold)。
9. 法術:地牢可施 Magic Missile/Kill/Ladder/Open/Unlock 等(接買來的法術)。
10. 酒館:花錢得線索(主線提示)。
11. 驗證:接任務→完成→領獎;施法;酒館線索。

> 每階段:實作 → game tester(進城鎮/買賣/戰鬥正常玩家路徑)→ 截圖目視 → commit。
> 數值優先 author 自手冊/wiki;若要逐版對齊原版,再反組譯 `ULTIMA.EXE` 校對(RE 為後備,非前置)。

## 5. 風險 / 待確認

- TCD.BIN 的 shop tile → 店家類型對應:先用 `TownSpriteType` 既有 tile 類型推斷,necessário 時對照原版進城實測。
- 原版精確數值(價格/傷害公式)：author 版先求「可玩、合理」,逐步對齊;不阻塞 Phase A。
- 城鎮 NPC 走動(非必要)可延後;先把**商店互動**這個最大缺口補上。
