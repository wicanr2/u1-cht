import sys
from PIL import Image
# 標準 16 色 RGBI palette
RGBI=[(0,0,0),(0,0,170),(0,170,0),(0,170,170),(170,0,0),(170,0,170),(170,85,0),(170,170,170),
      (85,85,85),(85,85,255),(85,255,85),(85,255,255),(255,85,85),(255,85,255),(255,255,85),(255,255,255)]
REV=[int(f"{b:08b}"[::-1],2) for b in range(256)]
path,out,rev = sys.argv[1],sys.argv[2],(len(sys.argv)>3 and sys.argv[3]=="rev")
d=open(path,"rb").read()[512:]
W=32; bpr=W//2
h=min(len(d)//bpr, 32*12)  # 前 12 個 32列
im=Image.new("RGB",(W,h))
p=0
for y in range(h):
    for xb in range(bpr):
        b=d[p]; p+=1
        if rev: b=REV[b]
        im.putpixel((xb*2,y),RGBI[(b>>4)&0xF])
        im.putpixel((xb*2+1,y),RGBI[b&0xF])
im.save(out); print("wrote",out)
