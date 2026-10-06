"""Register the revised right-handed fantasy bow set; keep BowsV1 untouched."""
from pathlib import Path
import json,shutil
import numpy as np
from PIL import Image,ImageFilter,ImageDraw
ROOT=Path(__file__).resolve().parents[1]
GEN=Path('C:/Users/tritl/.codex/generated_images/01a0bffb-e891-7110-bda7-4f3879f2880b')
OUT=ROOT/'ArtSource/BowsV2';OUT.mkdir(exist_ok=True)
FILES=['179f977b-387e-42dc-8fa4-e0a0e26684ce','1bdde89b-1f11-4c54-afbd-cfe68ccb1dc4','eba427dc-50df-460e-adcb-5dfcaaca90c1','e0432dac-bbce-4ced-9966-50998c5c027d','09c3c524-e7b4-4ae3-a60b-a68c07691456','544d883f-0bf2-44d8-a8f0-e34677872fb8','82d447f0-e9f5-4080-9275-4be81d0a6beb','60fca8e5-dee6-448e-bf83-2269c4b75465']
# Source-pixel arrow tips, measured per selected FULL-DRAW pose (not cell centers).
# N uses its final drawn pose as a short full-draw hold; the next source cell is release.
TIPS=[(1290,55),(495,626),(482,672),(502,692),(211,710),(88,720),(58,664),(38,637)]
# Measured lower-bow corridors in each source cell. These preserve the recurve
# below the tunic seam without retaining the original standing legs.
BOW_X=[[(335,395),(300,380),(300,380),(300,380),(315,380),(320,390)],
       [(265,370),(380,465),(385,465),(375,475),(380,470),(385,460)],
       [(270,355),(385,465),(365,455),(375,470),(415,490),(285,390)],
       [(295,385),(385,470),(365,465),(370,470),(400,490),(280,375)],
       [(155,250)]*6,
       [(120,290),(110,210),(100,205),(110,210),(105,205),(160,310)],
       [(135,225),(65,155),(65,155),(65,155),(65,155),(145,235)],
       [(145,235),(130,215),(130,215),(100,180),(100,185),(145,260)]]

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

def rgba(a):
    a=a.copy();a[a[:,:,3]==0,:3]=0;return Image.fromarray(a)

def isolate(a):
    groups=components(a[:,:,3]>64);main=max(groups,key=len)
    mask=np.zeros(a.shape[:2],np.uint8);yy,xx=np.array(main).T;mask[yy,xx]=255
    mask=np.array(Image.fromarray(mask).filter(ImageFilter.MaxFilter(5)))
    a[mask==0,3]=0;a[a[:,:,3]<12,3]=0
    return a

names=[];metrics=[];muzzles=[]
for d,id in enumerate(FILES):
    path=OUT/f'Source-D{d}.png';shutil.copy2(GEN/f'exec-{id}.png',path)
    sheet=np.array(Image.open(path).convert('RGBA'));h,w=sheet.shape[:2];cw=w//3;ch=h//2
    frames=[]
    for f in range(6):
        selected=2 if d==0 and f==3 else f
        cx=selected%3*cw;cy=selected//3*ch
        x0=max(0,cx-18);x1=min(w,cx+cw+22);y0=max(0,cy-12);y1=min(h,cy+ch)
        a=isolate(sheet[y0:y1,x0:x1].copy())
        rgb=a[:,:,:3].astype(float);gy,gx=np.mgrid[:a.shape[0],:a.shape[1]]
        skin=(a[:,:,3]>190)&(rgb[:,:,0]>rgb[:,:,1]*1.22)&(rgb[:,:,1]>65)&(rgb[:,:,2]>35)&(gy<ch*.43)&(gx>cw*.23)&(gx<cw*.75)
        head=max(components(skin),key=len);hy,hx=np.array(head).T
        top=int(hy.min());headx=float((hx.min()+hx.max())/2)
        foot=int(np.where(a[:,:,3]>100)[0].max());k=288/(foot-top)
        # Planted soles, rather than bow height, determine size and vertical registration.
        ox=round(240-headx*k);oy=round(432-foot*k)
        scaled=rgba(a).resize((round(a.shape[1]*k),round(a.shape[0]*k)),Image.Resampling.NEAREST)
        canvas=Image.new('RGBA',(576,576));canvas.paste(scaled,(ox,oy));frames.append(canvas)
        name=f'BowHero_{d}_{f}';canvas.save(OUT/f'{name}.png');names.append(name)
        upper=np.array(canvas);py=np.arange(576)[:,None]/3;px=np.arange(576)[None,:]/3;rgb=upper[:,:,:3].astype(float)
        cape=(rgb[:,:,1]>rgb[:,:,0]*1.05)&(rgb[:,:,1]>rgb[:,:,2]*1.18)
        bx0,bx1=BOW_X[d][f]
        source_x=(px*3-ox)/k+x0-cx
        corridor=(source_x>=bx0)&(source_x<=bx1)
        gold=(rgb[:,:,0]>130)&(rgb[:,:,1]>95)&(rgb[:,:,1]>rgb[:,:,0]*.63)&(rgb[:,:,0]>rgb[:,:,1]*1.07)
        # Dilated inlay pixels restore their dark steel outline, not nearby boots.
        bow_seed=(gold|cape)&corridor
        bow=np.array(Image.fromarray((bow_seed*255).astype('uint8')).filter(ImageFilter.MaxFilter(7)))>0
        bow&=corridor
        source_y=(py*3-oy)/k+y0-cy
        bow&=source_y<435
        upper[((py>=111)&(abs(px-80)<25)|(py>=116))&~(cape|bow),3]=0
        upper=isolate(upper)
        name=f'BowUpper_{d}_{f}';rgba(upper).save(OUT/f'{name}.png');names.append(name)
        metrics.append(dict(direction=d,frame=f,source_frame=selected,crop=[x0,y0,x1,y1],head=[headx+x0,top+y0],foot=foot+y0,scale=k,offset=[ox,oy]))
        if f==3:
            tx,ty=TIPS[d];muzzles.append([round((ox+(tx-x0)*k)/3,3),round((oy+(ty-y0)*k)/3,3)])
    # Mobile archery keeps the bow raised between shots; only planted archery
    # lowers it. Reuse the authored nocking-ready upper pose through these holds.
    for f in (0,5):shutil.copy2(OUT/f'BowUpper_{d}_1.png',OUT/f'BowUpper_{d}_{f}.png')
    preview=[]
    for im in frames:
        bg=Image.new('RGBA',im.size,(28,34,38,255));bg.alpha_composite(im);preview.append(bg.convert('RGB'))
    preview[0].save(OUT/f'Draw-D{d}.gif',save_all=True,append_images=preview[1:],duration=[120,160,180,190,150,200],loop=0)
    board=Image.new('RGB',(1440,260),(28,34,38));draw=ImageDraw.Draw(board)
    for f,im in enumerate(frames):
        im=im.resize((240,240),Image.Resampling.NEAREST);board.paste(im,(f*240,0),im);draw.text((f*240+4,244),str(f),fill='white')
    board.save(OUT/f'Review-D{d}.png')

for name in json.loads((ROOT/'ArtSource/BowsV1/manifest.json').read_text()):
    if name.startswith(('BowHero_','BowUpper_')):continue
    shutil.copy2(ROOT/'ArtSource/BowsV1'/f'{name}.png',OUT/f'{name}.png');names.append(name)
icon=GEN/'exec-2ce063bc-b9b9-4b50-a4c9-14d3a8582305.png';shutil.copy2(icon,OUT/'Source-Bow.png')
im=rgba(isolate(np.array(Image.open(icon).convert('RGBA'))));im=im.crop(im.getchannel('A').getbbox());im.thumbnail((232,232),Image.Resampling.LANCZOS)
canvas=Image.new('RGBA',(256,256));canvas.paste(im,((256-im.width)//2,(256-im.height)//2));canvas.save(OUT/'Loot_72.png')
(OUT/'manifest.json').write_text(json.dumps(names,indent=2))
(OUT/'registration.json').write_text(json.dumps(metrics,indent=2))
(OUT/'muzzles.json').write_text(json.dumps(muzzles))
(ROOT/'Source/TheBeardAndBlade/BowArtMetrics.h').write_text('#pragma once\n#include "CoreMinimal.h"\nnamespace BowArt {inline const FVector2D Muzzle[8]={'+','.join('{'+str(x)+'f,'+str(y)+'f}' for x,y in muzzles)+'};}\n')
print('Prepared',len(names),'fantasy bow assets;',muzzles)
