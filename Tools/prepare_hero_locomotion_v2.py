"""Mechanical extraction/registration of authored walk/run atlases; no painted frames.

Original Athletic_* artwork is never modified. Review the marker sheets and adjust
socket_overrides.json if a hidden hand cannot be resolved from the source artwork.
"""
from pathlib import Path
import json
import numpy as np
from PIL import Image, ImageDraw
from prepare_campaign_art import gutter, body_bounds

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / 'ArtSource/HeroLocomotionV2'
DST = ROOT / 'Content/Art/V2'

def components(mask):
    remaining = set(zip(*np.where(mask)))
    groups = []
    while remaining:
        seed = remaining.pop()
        stack, group = [seed], [seed]
        while stack:
            y, x = stack.pop()
            for dy, dx in ((-1,0),(1,0),(0,-1),(0,1)):
                q = y+dy, x+dx
                if q in remaining:
                    remaining.remove(q); stack.append(q); group.append(q)
        if len(group) >= 8:
            groups.append(np.array(group))
    return groups

def skin_mask(im):
    a = np.asarray(im).astype(np.int16)
    return (a[:,:,3]>128)&(a[:,:,0]>170)&(a[:,:,1]>75)&(a[:,:,0]>a[:,:,1]*1.4)&(a[:,:,1]>a[:,:,2]*1.05)

def extract(direction):
    im = Image.open(SRC/f'Direction{direction}.png').convert('RGBA')
    mask = np.asarray(im.getchannel('A'))>128
    assert (~mask).mean()>.15, 'Atlas must have real alpha, not a flattened background'
    ys = [0]+[gutter(mask.sum(1),round(r*im.height/4),round(im.height/4*.18)) for r in range(1,4)]+[im.height]
    frames=[]
    for r in range(4):
        xs=[0]+[gutter(mask[ys[r]:ys[r+1]].sum(0),round(c*im.width/4),round(im.width/4*.18)) for c in range(1,4)]+[im.width]
        for c in range(4):
            cell=im.crop((xs[c],ys[r],xs[c+1],ys[r+1]))
            bounds=body_bounds(np.asarray(cell.getchannel('A'))>128)
            assert bounds[0]>0 and bounds[1]>0 and bounds[2]<cell.width and bounds[3]<cell.height, (direction,r,c,'clipped frame')
            frames.append(cell.crop(bounds))
    scale=288/float(np.median([f.height for f in frames[:8]]))
    return frames, scale

def prepare():
    overrides=json.loads((SRC/'socket_overrides.json').read_text()) if (SRC/'socket_overrides.json').exists() else {}
    manifest=[]; all_sockets=[]; reports=[]
    for direction in range(8):
        if not (SRC/f'Direction{direction}.png').exists():
            continue
        frames,scale=extract(direction)
        sockets=[]; rendered=[]
        for i,raw in enumerate(frames):
            f=raw.resize((round(raw.width*scale),round(raw.height*scale)),Image.Resampling.NEAREST)
            skin=skin_mask(f)
            # Register on the head/torso axis instead of the cape bounding box.
            head=skin.copy();head[int(f.height*.28):]=False
            hy,hx=np.where(head)
            head_x=float(np.median(hx)) if len(hx) else f.width/2
            # Keep the torso centered while allowing a deliberate sprint lean.
            lean=(24 if direction in (1,2,3) else -24 if direction in (5,6,7) else 0) if i>=8 else 0
            px=round(192-head_x+lean)
            airborne=9 if i>=8 and i%4==2 else 0
            py=348-f.height-airborne
            assert px>=0 and px+f.width<=384 and py>=0, (direction,i,'canvas overflow',px,py,f.size)
            out=Image.new('RGBA',(384,384));out.paste(f,(px,py))
            state='Walk' if i<8 else 'Run';frame=i%8
            name=f'Locomotion_{state}_{direction}_{frame}'
            out.save(DST/f'{name}.png');manifest.append(name)
            # Find the anatomical right-hand skin island below the shoulders.
            hand_mask=skin.copy();hand_mask[:int(f.height*.40)]=False;hand_mask[int(f.height*.78):]=False
            groups=components(hand_mask)
            expected_x=f.width*(.83 if direction in (0,1,2,7) else .20)
            expected_y=f.height*(.52 if i>=8 else .61)
            if groups:
                g=min(groups,key=lambda g: abs(g[:,1].mean()-expected_x)+abs(g[:,0].mean()-expected_y)*1.3)
                hand=[round((px+float(g[:,1].mean()))/3,2),round((py+float(g[:,0].mean()))/3,2)]
            else:
                hand=[round((px+expected_x)/3,2),round((py+expected_y)/3,2)]
            hand=overrides.get(name,hand);sockets.append(hand);rendered.append(out)
            reports.append({'name':name,'socket':hand,'source_size':raw.size,'registered_bounds':out.getbbox()})
        board=Image.new('RGB',(4*384,4*410),(27,33,35));draw=ImageDraw.Draw(board)
        for i,out in enumerate(rendered):
            x,y=i%4*384,i//4*410
            board.paste(out,(x,y),out)
            sx,sy=sockets[i];sx=x+sx*3;sy=y+sy*3
            draw.ellipse((sx-5,sy-5,sx+5,sy+5),outline='cyan',width=2)
            draw.line((x+192,y,x+192,y+384),fill=(55,60,60))
            draw.text((x+8,y+384),f'D{direction} {"WALK" if i<8 else "RUN"} {i%8}',fill='white')
        board.save(SRC/f'Review{direction}.jpg',quality=95)
        # A mechanical preview of the actual extracted sequence, at gameplay size.
        for state,start in [('Walk',0),('Run',8)]:
            preview=[]
            for out in rendered[start:start+8]:
                canvas=Image.new('RGB',(192,192),(32,38,42))
                small=out.resize((192,192),Image.Resampling.NEAREST)
                canvas.paste(small,(0,0),small);preview.append(canvas)
            preview[0].save(SRC/f'{state}{direction}.gif',save_all=True,append_images=preview[1:],duration=95 if state=='Walk' else 74,loop=0)
        all_sockets.append((direction,sockets))
    (SRC/'manifest.json').write_text(json.dumps(manifest,indent=2))
    (SRC/'registration.json').write_text(json.dumps(reports,indent=2))
    if len(all_sockets)==8:
        rows=[]
        for _,sockets in all_sockets:
            rows.append(' {'+', '.join('{'+f'{x:.2f}f,{y:.2f}f'+'}' for x,y in sockets)+'}')
        header='#pragma once\n#include "CoreMinimal.h"\n// Generated by prepare_hero_locomotion_v2.py; normalized 128px right-hand anchors.\nnamespace HeroLocomotionSockets {\ninline const FVector2D Hand[8][16]={\n'+',\n'.join(rows)+'\n};\n}\n'
        (ROOT/'Source/TheBeardAndBlade/HeroLocomotionSockets.h').write_text(header)
    print(f'Prepared {len(manifest)} new frames; originals untouched.')

if __name__=='__main__':
    prepare()
