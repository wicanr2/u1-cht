# 原版 Ultima I (1981) 怪物 AI 行為考據

> 立檔緣由:實作 open_ultima 怪物行為前,先查證**原版 U1 的真實行為**,
> 避免憑空發明或誤套其他作品(如 U2/U3)的 AI。對齊本專案「反編當 oracle、不照抄、不臆造」原則。
> 考據日期:2026-06-26。

## 核心結論

| 場景 | 原版 U1 (1981) 怪物行為 |
|---|---|
| **地面(overworld)** | **不會移動**。怪物以**隨機遭遇**方式「出現在玩家所在位置並立即攻擊」,沒有接近 / 追擊 / 巡邏。 |
| **地牢(dungeon)** | **會移動並追蹤玩家**。地牢怪是隨機生成且會跟著玩家走 —— 與地面的靜止遭遇相反。 |
| **戰鬥節奏** | 回合制,且**有行動時限**(沒在時間內行動就喪失該回合)—— 承襲 Akalabeth。 |

> 一句話:**原版 U1「會動的怪」在地牢,不在地面。** 地面怪是「出現即打」的隨機遭遇,不追玩家。

## 對 open_ultima 實作的意涵(忠於原版)

| 場景 | open_ultima 現況 | 忠於原版的補法 | 不該做 |
|---|---|---|---|
| 地面 | 怪生成後**完全不攻擊**(只當靶子) | 怪**相鄰時反擊玩家**(無移動) | ❌ 加地面追擊 AI(那是 U2 行為,非 U1) |
| 地牢 | 有 Thief / GiantRat | 地牢怪**移動 / 追蹤玩家** | — |

⚠️ **誤區記錄**:u2-cht 的「逐玩家貼身追擊」AI 是從**《創世紀 2》反組譯 oracle** 推導的,
**不適用 U1 地面**。U1 地面怪不移動,套 U2 模型 = 偏離原版。

> 註:open_ultima 目前是把地面怪做成「持續存在於地圖、可走近砍」的形式(已與原版的
> 「隨機遭遇、出現即打」有出入)。在此前提下,**最小忠於原版**的增量是讓地面怪
> 「相鄰會反擊」而非「主動追擊」;真正的「移動追擊」留給地牢。

## 來源

- [Ultima I monster data — The Codex of Ultima Wisdom](https://wiki.ultimacodex.com/wiki/Ultima_I_monster_data)
- [The CRPG Addict — Game 4: Ultima (1981)](http://crpgaddict.blogspot.com/2010/02/game-4-ultima-i.html)
- [How to Make an RPG — Ultima I: The First Age of Darkness](https://howtomakeanrpg.com/r/l/g/ultima-1/)
- [Ultima I: The First Age of Darkness — Wikipedia](https://en.wikipedia.org/wiki/Ultima_I:_The_First_Age_of_Darkness)

## 待考據(若日後要更精準)

- 地面隨機遭遇的**觸發機率 / 地形相依**(原版是否依地形決定遇敵率)。
- 地牢怪的**移動規則**(每回合一格?是否純貪婪逐玩家?有無亂數?)— 實作地牢移動前再查 U1 反組譯 / Codex。
- 各怪種 HP / 攻擊力(見 Codex monster data 表)。
