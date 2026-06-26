set throttle off
proc press {row mask} { keymatrixdown $row $mask; after time 0.05 "keymatrixup $row $mask" }
proc grab {tag} {
  catch { set d [debug read_block "physical VRAM" 0 0x20000]
    set f [open "/work/re_work/msx/mv_$tag.bin" wb]; fconfigure $f -translation binary
    puts -nonewline $f $d; close $f }
  catch { screenshot -raw "/work/re_work/msx/k_$tag.png" }
}
after time 13 { type "OUT\r" }
# 不干擾,讓片頭自然播到選單;只在 t88 起敲 a
set t 88; for {set n 0} {$n<8} {incr n} { after time $t { type "a" }; set t [expr {$t+1.5}] }
after time 102 { type "AAA\r" }
# 分散分配:每項 RIGHT×6 然後 DOWN,共 6 項(t104-128)
set t 104
for {set s 0} {$s<6} {incr s} {
  for {set r 0} {$r<6} {incr r} { after time $t { press 8 0x80 }; set t [expr {$t+0.3}] }
  after time $t { press 8 0x40 }   ;# DOWN 下一項
  set t [expr {$t+0.5}]
}
after time 130 { grab alloc }
after time 131 { type " " }; after time 132 { type " " }
after time 133 { grab done }
# 種族/職業:猛敲
set t 135; for {set n 0} {$n<18} {incr n} { after time $t { type " " }; after time [expr {$t+0.4}] { type "\r" }; after time [expr {$t+0.8}] { type "a" }; set t [expr {$t+1.8}] }
after time 170 { type "b" }; after time 171 { type "\r" }
after time 172 { type "b" }; after time 173 { type "\r" }
set t 168; for {set n 0} {$n<24} {incr n} { after time $t [list grab [format ow%03d $t]]; after time [expr {$t+1}] { type " " }; set t [expr {$t+2}] }
after time 218 { exit }
