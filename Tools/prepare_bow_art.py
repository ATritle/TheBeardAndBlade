"""Crop and register approved generated bow art, preserving original game assets."""
from pathlib import Path
import json,shutil
from collections import deque
import numpy as np
from PIL import Image,ImageFilter,ImageDraw
ROOT=Path(__file__).resolve().parents[1]
GEN=Path('C:/Users/tritl/.codex/generated_images/01a0bffb-e891-7110-bda7-4f3879f2880b')
OUT=ROOT/'ArtSource/BowsV1';OUT.mkdir(exist_ok=True)
FILES=['bb4e2f29-11ca-4cc7-944d-6068990c193a','d226998e-5f54-4a12-a75b-970c4e190fec','57c8620f-fc70-4275-a97d-bbdcb3564e9d','66dcfdb0-af8b-44ce-be6a-2a1c2a5adec1','5374575e-8812-4ced-816e-d08aefc6d937','379ee7df-d14f-4c91-bec4-40f8198aa88d','e1a21ffe-0d47-44f2-87d6-562d4b31c3b9','5a95df03-884e-48b1-89cb-ae99e95ba056']
def components(mask):
    seen=mask.copy();groups=[]
    for y,x in zip(*np.where(mask)):
        if not seen[y,x]:continue
        q=[(int(y),int(x))];seen[y,x]=False;group=[]
        while q:
            yy,xx=q.pop();group.append((yy,xx))
            for ny,nx in ((yy-1,xx),(yy+1,xx),(yy,xx-1),(yy,xx+1)):
                if 0<=ny<seen.shape[0] and 0<=nx<seen.shape[1] and seen[ny,nx]:seen[ny,nx]=False;q.append((ny,nx))
        groups.append(group)
    return groups
def image(a):
    a=a.copy();a[a[:,:,3]==0,:3]=0;return Image.fromarray(a)
names=[];metrics=[];muzzles=[]
# Measured arrow-tip positions on the full-draw source pose. SE uses its corrected
# last drawn pose twice for the brief full-draw hold; its edited fourth cell is release.
tips=[(244,615),(464,601),(480,677),(1490,282),(290,710),(34,730),(52,673),(86,602)]
for d,source in enumerate(FILES):
    target=OUT/f'Source-D{d}.png';shutil.copy2(GEN/f'exec-{source}.png',target)
    sheet=np.array(Image.open(target).convert('RGBA'));h,w=sheet.shape[:2];cw=w//3;ch=h//2
    frames=[];direction_muzzle=None
    for f in range(6):
        selected=2 if d==3 and f==3 else f
        cx=selected%3*cw;cy=selected//3*ch
        # Expanded search regions plus connected alpha masks avoid neighboring-pose fragments.
        x0=max(0,cx-18);x1=min(w,cx+cw+22);y0=max(0,cy-12);y1=min(h,cy+ch)
        a=sheet[y0:y1,x0:x1].copy();groups=components(a[:,:,3]>64)
        main=max(groups,key=len);mask=np.zeros(a.shape[:2],np.uint8)
        yy,xx=np.array(main).T;mask[yy,xx]=255
        mask=np.array(Image.fromarray(mask).filter(ImageFilter.MaxFilter(5)))
        a[mask==0,3]=0;a[a[:,:,3]<12,3]=0
        # Locate the forehead, not the bow's upper tip, for consistent character scale.
        rgb=a[:,:,:3].astype(float);gy,gx=np.mgrid[:a.shape[0],:a.shape[1]]
        skin=(a[:,:,3]>190)&(rgb[:,:,0]>rgb[:,:,1]*1.22)&(rgb[:,:,1]>65)&(rgb[:,:,2]>35)&(gy<ch*.43)&(gx>cw*.23)&(gx<cw*.72)
        head=max(components(skin),key=len);hy,hx=np.array(head).T
        head_top=int(hy.min());head_x=float((hx.min()+hx.max())/2)
        foot=int(np.where(a[:,:,3]>100)[0].max())
        k=288/(foot-head_top) # 96 logical px, like original adventurer.
        im=image(a);scaled=im.resize((round(im.width*k),round(im.height*k)),Image.Resampling.NEAREST)
        ox=round(240-head_x*k);oy=round(432-foot*k)
        canvas=Image.new('RGBA',(576,576));canvas.paste(scaled,(ox,oy))
        name=f'BowHero_{d}_{f}';canvas.save(OUT/f'{name}.png');names.append(name);frames.append(canvas)
        upper=np.array(canvas);py=np.arange(576)[:,None]/3;px=np.arange(576)[None,:]/3;rgb=upper[:,:,:3].astype(float)
        cape=(rgb[:,:,1]>rgb[:,:,0]*1.05)&(rgb[:,:,1]>rgb[:,:,2]*1.18)
        upper[(py>=111)&(abs(px-80)<25)&~cape,3]=0
        # Outside the hip strip, generated stepping boots can extend sideways.
        # Only the cloak continues below the lower bow/hip silhouette.
        upper[(py>=116)&~cape,3]=0
        name=f'BowUpper_{d}_{f}';image(upper).save(OUT/f'{name}.png');names.append(name)
        metrics.append(dict(name=f'BowHero_{d}_{f}',source_frame=selected,crop=[x0,y0,x1,y1],head=[head_x+x0,head_top+y0],foot=foot+y0,scale=k,offset=[ox,oy]))
        if f==3:
            tx,ty=tips[d];direction_muzzle=[round((ox+(tx-x0)*k)/3,3),round((oy+(ty-y0)*k)/3,3)]
    muzzles.append(direction_muzzle)
    preview=[]
    for im in frames:
        b=Image.new('RGBA',im.size,(28,34,38,255));b.alpha_composite(im);preview.append(b.convert('RGB'))
    preview[0].save(OUT/f'Draw-D{d}.gif',save_all=True,append_images=preview[1:],duration=[120,160,180,190,150,200],loop=0)
    board=Image.new('RGB',(6*240,260),(28,34,38));draw=ImageDraw.Draw(board)
    for f,im in enumerate(frames):im=im.resize((240,240),Image.Resampling.NEAREST);board.paste(im,(f*240,0),im);draw.text((f*240+4,244),str(f),fill='white')
    board.save(OUT/f'Review-D{d}.png')
def save_tile(im,name,size=256):
    box=im.getchannel('A').point(lambda a:255 if a>24 else 0).getbbox();assert box,name
    im=im.crop(box);im.thumbnail((size-24,size-24),Image.Resampling.LANCZOS)
    canvas=Image.new('RGBA',(size,size));canvas.paste(im,((size-im.width)//2,(size-im.height)//2));canvas.save(OUT/f'{name}.png');names.append(name)
icons=OUT/'Source-Bows.png';shutil.copy2(GEN/'exec-d0a54e41-5dc8-432e-a62b-d6dff7144b7f.png',icons)
im=Image.open(icons);w,h=im.size
for i in range(5):save_tile(im.crop((i%3*w//3,i//3*h//2,(i%3+1)*w//3,(i//3+1)*h//2)),f'Loot_{72+i}')
fx=OUT/'Source-Effects.png';shutil.copy2(GEN/'exec-70b0610d-4747-4b3e-843f-23db72e3e29b.png',fx)
im=Image.open(fx);w,h=im.size
# Authored atlas has a wider projectile column; these are measured gutters, not uniform slicing.
xs=[0,.268,.427,.615,.815,1];ys=[0,.21,.41,.595,.79,1]
for row in range(5):
    for col in range(5):
        rect=(int(xs[col]*w),int(ys[row]*h),int(xs[col+1]*w),int(ys[row+1]*h))
        save_tile(im.crop(rect),f'BowFlight_{row}' if col==0 else f'BowImpact_{row}_{col-1}')
(OUT/'manifest.json').write_text(json.dumps(names,indent=2));(OUT/'registration.json').write_text(json.dumps(metrics,indent=2))
(ROOT/'Source/TheBeardAndBlade/BowArtMetrics.h').write_text('#pragma once\n#include "CoreMinimal.h"\nnamespace BowArt {inline const FVector2D Muzzle[8]={'+','.join('{'+str(x)+'f,'+str(y)+'f}' for x,y in muzzles)+'};}\n')
shutil.copy2(ROOT/'Tools/bow-art-prompts.json',OUT/'prompts.json')
print(len(names),'bow assets prepared; muzzle positions',muzzles)
