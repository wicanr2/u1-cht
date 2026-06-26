#!/usr/bin/env python3
"""組 FM Towns U1 PNG AssetPack(832×16,對齊 engine 52 槽)。
解碼:chunky 4bpp + FillOrder=2 反序 + 亮 RGB-16(見 docs/re/fmtowns-u1-graphics.md)。
UT1MAP=地形(自動分類)+ UT1TILE1=物件 + UT1TILE0=怪物;32×32 → downscale 16×16。
用法:build_fmtowns_pack.py <GRAPH目錄> <out.png>"""
import sys
from PIL import Image
PAL=[(0,0,0),(0,0,255),(0,255,0),(0,255,255),(255,0,0),(255,0,255),(255,255,0),(255,255,255)]*2
REV=[int(f"{b:08b}"[::-1],2) for b in range(256)]

def tiles(path):
    d=open(path,"rb").read()[512:]; W=32; bpr=W//2
    nt=len(d)//(32*bpr); out=[]
    for t in range(nt):
        im=Image.new("RGB",(W,W))
        for y in range(W):
            for xb in range(bpr):
                b=REV[d[t*32*bpr+y*bpr+xb]]
                im.putpixel((xb*2,y),PAL[(b>>4)&0xF]); im.putpixel((xb*2+1,y),PAL[b&0xF])
        out.append(im)
    return out

def cls(im):  # 主色分類
    from collections import Counter
    c=Counter(im.getdata())
    return c

def main():
    graph,out=sys.argv[1],sys.argv[2]
    mp=tiles(f"{graph}/UT1MAP.TIF")
    t1=tiles(f"{graph}/UT1TILE1.TIF")
    t0=tiles(f"{graph}/UT1TILE0.TIF")
    # 自動找地形(主色比例)
    def frac(im,cols):
        c=cls(im); tot=sum(c.values())
        return sum(n for col,n in c.items() if col in cols)/tot
    BLUE={(0,0,255),(0,255,255)}; GREEN={(0,255,0)}; BLACK={(0,0,0)}
    cand=range(min(24,len(mp)))
    terr={}
    terr['water']=max(cand, key=lambda i:frac(mp[i],BLUE))
    terr['grass']=max(cand, key=lambda i:frac(mp[i],GREEN)-frac(mp[i],BLACK))
    terr['forest']=max(cand, key=lambda i:(frac(mp[i],GREEN) if 0.05<frac(mp[i],BLACK)<0.5 else 0))
    terr['mountain']=max(cand, key=lambda i:(frac(mp[i],BLACK) if 0.3<frac(mp[i],BLACK)<0.85 else 0))
    print("terrain tile idx:",terr,
          "  water blue%=", round(frac(mp[terr['water']],BLUE),2))
    # 組 52 槽:0water 1grass 2forest 3mountain;4+ 用 t1 物件;player+怪用 t0
    pick=[mp[terr['water']],mp[terr['grass']],mp[terr['forest']],mp[terr['mountain']]]
    # 4-9: castle,castle,signpost,town,town,dungeon → 從 t1 取前幾個物件(provisional)
    obj=[t for t in t1[:48]]
    mon=[t for t in t0[:48]]
    seq=pick + obj[:6] + [t0[0]] + obj[6:6+6] + mon[:48]  # 對齊 52
    grid=Image.new("RGB",(16*52,16),(0,0,0))
    for i in range(52):
        src=seq[i] if i<len(seq) else pick[1]
        grid.paste(src.resize((16,16),Image.NEAREST),(i*16,0))
    grid.save(out); print("wrote",out)
if __name__=="__main__": main()
