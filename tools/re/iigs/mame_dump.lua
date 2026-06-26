-- mame_dump.lua — MAME apple2gs autoboot script:多時點 dump SHR 螢幕記憶體
-- 用法(在 u1-mame docker 內):
--   /usr/games/mame -rompath <bios> apple2gs \
--     -flop3 <GS/OS開機碟.2mg> -flop4 <Ultima.woz> \
--     -autoboot_script mame_dump.lua -video none -nothrottle -seconds_to_run N
-- 產出:/work/re_work/iigs/mame/shr_<秒>.bin → 用 tools/re/iigs/render_shr.py 渲染
-- SHR 記憶體 = bank $E1 的 $2000–$9FFF(maincpu program space 0xE12000–0xE19FFF)。
local mac = manager.machine
local sp
local function dump(tag)
  if not sp then sp = mac.devices[":maincpu"].spaces["program"] end
  local f = io.open("/work/re_work/iigs/mame/shr_" .. tag .. ".bin", "wb")
  for a = 0xE12000, 0xE19FFF do f:write(string.char(sp:read_u8(a))) end
  f:close(); print("DUMP " .. tag)
end
local cnt = 0
-- 在 30/60/90/120 秒各 dump 一次(調整以對齊 overworld 出現時機)
local marks = {1800, 3600, 5400, 7200}
emu.register_frame_done(function()
  cnt = cnt + 1
  for _, fr in ipairs(marks) do
    if cnt == fr then dump(tostring(fr // 60)) end
  end
end)
-- 鍵盤注入(導航建角等):用 mac:ioport 或 natural keyboard
-- 例:emu.keypost("text\n") 在 GS/OS / 遊戲提示處輸入;滑鼠用 :mouse port(Finder 啟動 app 需滑鼠)
