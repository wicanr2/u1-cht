# 多時間點 dump:VRAM + palette + 截圖,觀察開機到載入
proc grab {tag} {
  debug save_to_file "physical VRAM" 0 0x20000 /work/re_work/msx/vram_$tag.bin
  catch { set_screenshot /work/re_work/msx/scr_$tag.png } 
}
after time 4  { grab t4 }
after time 8  { grab t8 }
after time 12 { grab t12 }
after time 12.5 { exit }
