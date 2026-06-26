after time 14 {
  set f [open "/work/re_work/msx/debuglist.txt" w]
  puts $f [debug list]
  catch { puts $f "VDP-REG: [debug read_block {VDP regs} 0 64]" }
  close $f
  exit
}
