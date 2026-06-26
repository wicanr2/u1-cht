#!/usr/bin/env python3
"""組 FM Towns U1 PNG AssetPack(832×16,對齊 engine 52 槽)。
解碼:chunky 4bpp + FillOrder=2 反序 + 亮 RGB-16(見 docs/re/fmtowns-u1-graphics.md)。
UT1MAP=地形(自動分類)+ UT1TILE1=物件 + UT1TILE0=怪物;32×32 → downscale 16×16。
用法:build_fmtowns_pack.py <GRAPH目錄> <out.png>"""
import sys
from PIL import Image
# FM Towns U1 只用偶數 index(=8 色)。PAL[2j]=color[j]。
# 依 hg101 實機色 + tile index 分析推:綠(grass idx2)/青藍(water idx8/10)/白(紋理 idx12)。
_C8=[(0,0,0),(0,255,0),(255,0,0),(200,120,0),(0,255,255),(0,0,255),(255,255,255),(255,255,0)]
PAL=[(0,0,0)]*16
for _j in range(8): PAL[2*_j]=_C8[_j]
for _j in range(8): PAL[2*_j+1]=_C8[_j]   # 奇數 index 罕用,填同色
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
    # === 52 槽明確對應(engine 順序見 TileTypeLoader)===
    # 來源:W/G/F/M=UT1MAP(自動);物件/載具=UT1TILE1;玩家/怪物=UT1TILE0(視覺辨識)
    # UT1TILE1:24-25城堡 0招牌 26-29馬 30-33車 34-37筏 38-43船 44-47列車 48-51梭
    # UT1TILE0:0玩家,8+各種怪
    W={'w':mp[terr['water']],'g':mp[terr['grass']],'f':mp[terr['forest']],'m':mp[terr['mountain']]}
    def T1(i): return t1[i] if i<len(t1) else t1[0]
    def T0(i): return t0[i] if i<len(t0) else t0[0]
    seq=[
        W['w'],W['g'],W['f'],W['m'],          # 0-3 water grass forest mountain
        T1(24),T1(25),                         # 4-5 castle(2幀)
        T1(0),                                 # 6 signpost(招牌/星)
        mp[6],mp[6],                           # 7-8 town(2幀)= UT1MAP 城門
        T1(13),                                # 9 dungeon entrance(暫用建築)
        T0(0),                                 # 10 player
        T1(26),                                # 11 horse
        T1(30),                                # 12 cart
        T1(34),                                # 13 raft
        T1(40),T1(41),                         # 14-15 frigate(2幀)
        T1(44),                                # 16 aircar
        T1(48),                                # 17 shuttle
        T1(52),                                # 18 time machine
    ]
    # 19-51 怪物(33 槽,多為2幀)→ UT1TILE0 怪物序列(跳過 player tile0)
    mi=8
    while len(seq)<52:
        seq.append(T0(mi)); mi=(mi+1)% len(t0)
        if mi==0: mi=8
    # RGBA:slot>=10(玩家/載具/怪物=疊在地形上的)黑底→透明;0-9 地形/結構保持不透明
    grid=Image.new("RGBA",(16*52,16),(0,0,0,0))   # 全透明底,透明 sprite 像素不被蓋黑
    for i in range(52):
        tile=seq[i].resize((16,16),Image.NEAREST).convert("RGBA")
        if i>=10:
            px=tile.load()
            for y in range(16):
                for x in range(16):
                    r,g,b,a=px[x,y]
                    if r==0 and g==0 and b==0: px[x,y]=(0,0,0,0)
        grid.paste(tile,(i*16,0),tile)
    grid.save(out); print("wrote",out,"slots",len(seq))
if __name__=="__main__": main()
