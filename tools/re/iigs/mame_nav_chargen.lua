-- mame_nav_chargen.lua — MAME apple2gs:自動把 Ultima I IIgs 驅動過開機+角色創建
-- (已驗證可達 GS/OS 載入對話框;最後一步「載入存檔」需滑鼠,見 docs/re/e2-apple-iigs.md)
--
-- 跑法(u1-mame docker,-video none 快 ~8×):
--   mame -rompath <bios> apple2gs \
--     -flop3 "<GS/OS System 6.0.2 Disk2 .2mg>" -flop4 "<Ultima I IIgs.woz>" \
--     -autoboot_script mame_nav_chargen.lua -video none -nothrottle -seconds_to_run 900
--
-- 已驗證完整遊戲流程(emulated frame@60fps;GS/OS 開機 ~150-270s,變異大):
--   1. GS/OS 從 flop3 開機到 Finder(桌面 + System Disk/Ultima I 圖示)
--   2. 鍵盤 'u'(選 Ultima I 碟)+ Cmd+O ×2 → 啟動 ULTIMAI(ADB 鍵盤緩衝,早送也生效)
--   3. Tool035 MidiSynth 警告對話框 → Return/Space/Enter 之一可關
--   4. 主選單「A) Generate new / B) Continue previous save」→ 按 A
--   5. Character Generation:Right=加屬性、Down=換屬性(單屬性上限~25,需分散),分配 30 點後 Space
--   6. race(a-d)/sex(a-b)/class(a-d) 各按字母;Enter thy name:打字 + Return
--   7. 「Save this character? (Y-N)」→ Y → GS/OS「Save thy game as...」存檔對話框
--   8. ⚠ 存檔對話框 SFPutFile:Return 有時可按 Save(時序敏感)→ 回主選單
--   9. B → 「Which game my Lord?」GS/OS SFGetFile 載入對話框
--  10. ⛔ 載入對話框需滑鼠雙擊存檔(鍵盤 Down/Return/type-select 皆無法選檔開啟)
--
-- 滑鼠(ADB 相對座標):MOUSE0=button, MOUSE1=X, MOUSE2=Y;field:set_value(delta) 每 frame 累加。
-- pin 到角落(X=-30 連續 60f)已驗證有效;移到目標需校準 delta→pixel(640 模式 QuickDraw 座標)。
local mac=manager.machine
local P=mac.ioport.ports
local F={
  u=P[":macadb:KEY2"].fields["u  U"], o=P[":macadb:KEY1"].fields["o  O"],
  cmd=P[":macadb:KEY3"].fields["Command / Open Apple"], ret=P[":macadb:KEY2"].fields["Return"],
  a=P[":macadb:KEY0"].fields["a  A"], spc=P[":macadb:KEY3"].fields["Space"],
  right=P[":macadb:KEY3"].fields["Right Arrow"], down=P[":macadb:KEY3"].fields["Down Arrow"],
  y=P[":macadb:KEY1"].fields["y  Y"],
}
local acts={}
local function tap(f,fr,dur) table.insert(acts,{f,fr,fr+(dur or 6)}) end
for _,base in ipairs({9000,9900,10800,11700}) do tap(F.u,base); tap(F.cmd,base+120,30); tap(F.o,base+128) end
for _,fr in ipairs({12600,13200,13800}) do tap(F.ret,fr) end
tap(F.a,14400)
local fr=15300
for attr=1,6 do for i=1,5 do tap(F.right,fr,8); fr=fr+22 end; if attr<6 then tap(F.down,fr,8); fr=fr+22 end end
fr=fr+120; tap(F.spc,fr,10)
tap(F.a,fr+1200); tap(F.a,fr+2400); tap(F.a,fr+3600); tap(F.a,fr+4800); tap(F.ret,fr+5000)
tap(F.y,fr+6000)
local prev={}
local function setf(f,v) if prev[f]~=v then f:set_value(v); prev[f]=v end end
local function dump(tag)
  local sp=mac.devices[":maincpu"].spaces["program"]
  local fp=io.open("/work/re_work/iigs/mame/shr_"..tag..".bin","wb")
  for a=0xE12000,0xE19FFF do fp:write(string.char(sp:read_u8(a))) end; fp:close()
end
local cnt=0
emu.register_frame_done(function()
  cnt=cnt+1
  for _,f in pairs(F) do
    local on=false
    for _,a in ipairs(acts) do if a[1]==f and cnt>=a[2] and cnt<=a[3] then on=true end end
    setf(f,on and 1 or 0)
  end
  if cnt>=15600 and cnt%600==0 then dump(tostring(cnt//60).."s") end
end)
