set throttle off
set PROG "/work/re_work/msx/Ultima I - The First Age of Darkness (1986)(Pony Canyon)(ja).dsk"
set CHAR "/work/re_work/msx/CHARDISK.dsk"
proc press {row mask} { keymatrixdown $row $mask; after time 0.05 "keymatrixup $row $mask" }
proc grab {tag} {
  catch { set d [debug read_block "physical VRAM" 0 0x20000]
    set f [open "/work/re_work/msx/sv_$tag.bin" wb]; fconfigure $f -translation binary
    puts -nonewline $f $d; close $f }
  catch { screenshot -raw "/work/re_work/msx/s_$tag.png" }
}
after time 13 { type "OUT\r" }
set t 88; for {set n 0} {$n<8} {incr n} { after time $t { type "a" }; set t [expr {$t+1.5}] }
after time 102 { type "AAA\r" }
# 分散分配 30 點
set t 104
for {set s 0} {$s<6} {incr s} {
  for {set r 0} {$r<6} {incr r} { after time $t { press 8 0x80 }; set t [expr {$t+0.3}] }
  after time $t { press 8 0x40 }; set t [expr {$t+0.5}]
}
after time 130 { type " " }; after time 131 { type " " }
# 種族/職業猛敲
set t 133; for {set n 0} {$n<14} {incr n} { after time $t { type " " }; after time [expr {$t+0.4}] { type "\r" }; after time [expr {$t+0.8}] { type "a" }; set t [expr {$t+1.6}] }
# 到「插入角色碟」→ 換空白碟 + space(多次嘗試)
after time 158 { diska $CHAR }
after time 159 { type " " }
after time 161 { type " " }
after time 163 { grab charsaved }
# 換回程式碟 + space(進 overworld 載模組)
after time 165 { diska $PROG }
after time 166 { type " " }
after time 168 { type " " }
# dump 全程找 overworld
set t 168; for {set n 0} {$n<22} {incr n} { after time $t [list grab [format ow%03d $t]]; after time [expr {$t+1}] { type " " }; set t [expr {$t+2}] }
after time 214 { exit }
