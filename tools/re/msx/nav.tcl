proc dump {tag} {
  catch { screenshot -raw "/work/re_work/msx/n_$tag.png" }
  foreach {blk fn} {{physical VRAM} vram {VDP palette} pal {VDP regs} regs} {
    catch {
      set d [debug read_block $blk 0 [debug size $blk]]
      set f [open "/work/re_work/msx/${fn}_$tag.bin" wb]; fconfigure $f -translation binary
      puts -nonewline $f $d; close $f
    }
  }
}
after time 13 { type "OUT\r" }
# 過 title + 建角:交替敲 space/return + 常見鍵,每 2s 截圖
set t 18
foreach k {{ } "\r" { } "\r" A "\r" { } "\r" Y "\r" { } "\r" { } "\r"} {
  after time $t [list type $k]
  after time [expr {$t+0.8}] [list dump s$t]
  set t [expr {$t+2}]
}
after time [expr {$t+1}] { exit }
