set throttle off
proc grab {tag} {
  catch { screenshot -raw "/work/re_work/msx/p_$tag.png" }
  catch { set d [debug read_block "physical VRAM" 0 0x20000]
    set f [open "/work/re_work/msx/pv_$tag.bin" wb]; fconfigure $f -translation binary
    puts -nonewline $f $d; close $f }
}
after time 13 { type "OUT\r" }
# 衝過片頭到選單(連敲 space)
set t 16
for {set n 0} {$n<40} {incr n} { after time $t { type " " }; set t [expr {$t+2}] }
# 到選單,敲 b 遊戲開始
after time 98  { type "b" }
after time 99  { type "\r" }
after time 101 { grab b101 }
after time 104 { grab b104 }
after time 108 { type " " }
after time 110 { grab b110 }
after time 114 { type "a" }
after time 116 { grab a116 }
after time 116.5 { exit }
