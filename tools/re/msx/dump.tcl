proc grab {tag} {
  set data [debug read_block "physical VRAM" 0 0x20000]
  set f [open "/work/re_work/msx/vram_$tag.bin" wb]
  fconfigure $f -translation binary
  puts -nonewline $f $data
  close $f
  catch { screenshot -raw "/work/re_work/msx/scr_$tag.png" }
}
after time 4  { grab t4 }
after time 8  { grab t8 }
after time 12 { grab t12 }
after time 12.5 { exit }
