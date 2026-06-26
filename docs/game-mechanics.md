# 遊戲機制:時間 tick 與怪物生成(參考 u2-cht)

## 背景:上游 open_ultima 的生怪是壞的占位
- 原本以 **real-time** 計時(`npcSpawnCounter += elapsed`,每 ~10ms),且生在玩家
  **SE +19 格(畫面外)**、`activateNpcs()` 是空 stub(怪不會移動)→ 怪永遠看不到。

## 改為 turn-based(對齊 u2-cht)
每**走一步**觸發 `OverworldScreen::onStep()`:

```
onStep():
  spawnNpcs()                       // 每步嘗試生怪(spawn_pct 機率)
  time_accum += speed_pct           // 時間累加器
  while time_accum >= 100:          // 滿 100 走一個時間 tick
      time_accum -= 100
      (future) step_mobs / 食物消耗  // 上游尚無,先建節奏
```

### 生怪 `spawnNpcs()`
- 場上怪數 ≥ `MOB_MAX`(8)→ 不生。
- 機率閘門:`spawn_pct < 100 且 rand%100 >= spawn_pct` → 放棄;再 `rand%8 >= 7`(≈12.5% 基礎放棄,對齊 u2/oracle)。
- 在**視野內**隨機可通行(非山非水)空格生成,離玩家 ≥3 格(不貼臉)。

### 可調參數(`config.json`)
| 鍵 | 預設 | 作用 |
|---|---|---|
| `speed_pct` | 100 | 時間流速 %(100=每步一 tick) |
| `spawn_pct` | 55 | 每步生怪機率倍率(淨 ≈ 0.875×0.55 ≈ 48%/步) |

## 切換 tileset 時的 NPC 處理(u4-cht 曾踩雷:NPC 消失)
- 切 tileset 會呼叫 `OverworldScreen::init()` 重建 `_spritesMap`(新材質)。
- **修法**:`init()` 末尾把現存 `_enemies` 每隻的 `OverworldTile` sprite 重新指向新
  `_spritesMap` 同型 sprite(`OverworldTile::setSprite`)。
  → NPC **不消失**且外觀同步切到新 tileset(避免保留舊 palette)。
- 驗證:`t06`(EGA 兩隻術士)→ `t07`(CGA 後兩隻仍在且轉洋紅)。

## 後續
- `step_mobs()`:怪物逐玩家移動 + 貼身攻擊(目前 `activateNpcs` 仍空)。
- 食物消耗 / 飢餓扣血掛在時間 tick。
- F6 即時調整 speed/spawn(目前走 config.json)。
