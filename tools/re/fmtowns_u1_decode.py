import sys
from PIL import Image
PAL=[(0,0,0),(0,0,255),(0,255,0),(0,255,255),(255,0,0),(255,0,255),(255,255,0),(255,255,255),
     (0,0,0),(0,0,255),(0,255,0),(0,255,255),(255,0,0),(255,0,255),(255,255,0),(255,255,255)]
REV=[int(f"{b:08b}"[::-1],2) for b in range(256)]
d=open(sys.argv[1],"rb").read()[512:]
W=32; bpr=W//2
nt=len(d)//(32*bpr)
# 並排成網格:每列 8 tile
cols=8; rows=(nt+cols-1)//cols
grid=Image.new("RGB",(cols*(W+2), rows*(W+2)),(40,40,40))
for t in range(nt):
    tile=Image.new("RGB",(W,W))
    for y in range(W):
        for xb in range(bpr):
            b=REV[d[t*32*bpr + y*bpr + xb]]
            tile.putpixel((xb*2,y),PAL[(b>>4)&0xF]); tile.putpixel((xb*2+1,y),PAL[b&0xF])
    gx=(t%cols)*(W+2)+1; gy=(t//cols)*(W+2)+1
    grid.paste(tile,(gx,gy))
grid.save(sys.argv[2]); print(f"wrote {sys.argv[2]}  {nt} tiles, grid {cols}x{rows}")
