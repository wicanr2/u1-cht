set throttle off
proc grab {tag} {
  catch { screenshot -raw "/work/re_work/msx/r_$tag.png" }
  catch { set d [debug read_block "physical VRAM" 0 0x20000]
    set f [open "/work/re_work/msx/vr_$tag.bin" wb]; fconfigure $f -translation binary
    puts -nonewline $f $d; close $f }
}
after time 13 { type "OUT\r" }
set t 16
for {set n 0} {$n<60} {incr n} {
  after time $t { type " " }
  after time [expr {$t+0.6}] { type "\r" }
  after time [expr {$t+1.3}] [list grab [format %03d [expr {int($t)}]]]
  set t [expr {$t+2}]
}
after time [expr {$t+1}] { exit }
