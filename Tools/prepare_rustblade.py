"""Mechanical extraction only; authored pose pixels are not painted/interpolated.

Original crops/pivots are authoritative, including the corrected SW rectangles.
Generated state sheets use measured alpha gutters, never equal-height slices.
"""
import json
from pathlib import Path
from statistics import median
from collections import deque
from functools import lru_cache
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'ArtSource/EnemyExpansion/RustbladeSquire'
OUT = ROOT / 'Content/Art/EnemyExpansion/RustbladeSquire'
REVIEW = SOURCE / 'runtime-v2'
DIRS = ['N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW']
SIZE, ROOT_X, ROOT_Y, BODY_HEIGHT = 384, 192, 340, 220
# SW's more crouched, broad source drawing was enlarged by standing-height
# normalization. Match its anatomical size, not its total silhouette height.
# Apply to every SW state around the same ground pivot to avoid state-change pops.
DIRECTION_SCALE = {'SW': .90}
WALKS = {
 'N':[0,1,3,5,6,8,9,10], 'NE':[0,3,5,6,7,9,10,11],
 'E':[0,2,3,5,6,7,8,9], 'SE':[0,1,3,5,6,7,10,11],
 'S':[0,3,6,7,8,9,10,11], 'SW':[0,2,3,6,8,10,11,14],
 'W':[0,1,2,3,8,9,10,11], 'NW':[0,2,4,6,8,10,12,14],
}
# Remove duplicate preparation/recovery and the second (spurious) wind-up.
# Position 6 is the first downstroke, not a blindly copied atlas frame number.
ATTACKS = {
 'N': [0,3,5,7,8,10,11,12,14,15,20,22,23],
 'NE':[0,3,4,6,8,9,10,11,12,14,20,22,23],
 'E': [0,3,4,6,8,9,10,11,12,15,16,20,23],
 'SE':[0,3,5,6,8,9,10,11,12,14,16,20,23],
 'S': [0,3,4,5,7,9,10,11,12,14,15,20,23],
 'SW':[0,3,4,6,8,10,12,13,14,15,18,22,23],
 'W': [0,3,4,6,8,9,12,13,14,15,16,20,23],
 'NW':[0,3,5,6,8,10,12,13,14,15,16,20,23],
}

def gutter(profile, target, radius):
    lo, hi = max(1, round(target-radius)), min(len(profile)-1, round(target+radius))
    values = profile[lo:hi]
    points = np.flatnonzero(values == values.min()) + lo
    runs = np.split(points, np.where(np.diff(points) != 1)[0]+1)
    return round(float(min(runs, key=lambda r: abs(r.mean()-target)).mean()))

def state_cells(im, rows=4):
    a = np.asarray(im)[:,:,3] > 64
    h,w = a.shape
    # Independent column row gutters avoid a blade crossing the next row's cut.
    xs = [0]+[gutter(a.sum(axis=0),w*c/4,w*.035) for c in range(1,4)]+[w]
    frames = [None]*(4*rows)
    for col in range(4):
        x0,x1=xs[col:col+2]
        ys=[0]+[gutter(a[:,x0:x1].sum(axis=1),h*r/rows,h/rows*.28) for r in range(1,rows)]+[h]
        for row in range(rows):
            y0,y1=ys[row:row+2]
            bounds=Image.fromarray(a[y0:y1,x0:x1]).getbbox()
            assert bounds, (col,row)
            frames[row*4+col]={'cell':[x0,y0,x1-x0,y1-y0], 'bounds':list(bounds)}
    return frames

def cut(im, meta):
    x,y,w,h=meta['cell']
    crop=im.crop((x,y,x+w,y+h))
    a=np.array(crop.getchannel('A'))
    # Remove sub-visible alpha background, not brown armor RGB.
    a[a<32]=0
    l,t,r,b=meta['bounds']
    a[:t]=0;a[b:]=0;a[:,:l]=0;a[:,r:]=0
    # Honor per-scanline masks if future supplied revisions contain them.
    for row in meta.get('clipRows',[]):
        yy,left,right=row
        a[yy,:left]=0;a[yy,right:]=0
    # Neighbor-pose fragments in flagged original cells are disconnected from
    # the character. Join only two-pixel sprite seams, then retain the primary
    # connected silhouette. This is an alpha crop mask, not generated artwork.
    if meta.get('edgeRisk'):
        mask=np.asarray(Image.fromarray((a>64).astype('uint8')*255).filter(ImageFilter.MaxFilter(5)))>0
        visited=np.zeros_like(mask);components=[]
        for yy,xx in zip(*np.nonzero(mask)):
            if visited[yy,xx]:continue
            queue=deque([(yy,xx)]);visited[yy,xx]=True;component=[]
            while queue:
                cy,cx=queue.popleft();component.append((cy,cx))
                for dy,dx in ((0,1),(0,-1),(1,0),(-1,0)):
                    ny,nx=cy+dy,cx+dx
                    if 0<=ny<h and 0<=nx<w and mask[ny,nx] and not visited[ny,nx]:
                        visited[ny,nx]=True;queue.append((ny,nx))
            components.append(component)
        keep=np.zeros_like(mask)
        if components:
            ys,xs=zip(*max(components,key=len));keep[ys,xs]=True
            a[~keep]=0
    crop.putalpha(Image.fromarray(a))
    return crop

def aligned(im,meta,scale,anchor_x=None):
    crop=cut(im,meta)
    l,t,r,b=meta['bounds']
    px=meta.get('pivotX',anchor_x if anchor_x is not None else crop.width/2)
    # The shared ground root is independent of the blade, pose width and death height.
    py=crop.getchannel('A').point(lambda v:255 if v>64 else 0).getbbox()[3]-1
    scaled=crop.resize((round(crop.width*scale),round(crop.height*scale)),Image.Resampling.NEAREST)
    canvas=Image.new('RGBA',(SIZE,SIZE))
    dx,dy=round(ROOT_X-px*scale),round(ROOT_Y-py*scale)
    bb=scaled.getbbox()
    assert bb and dx+bb[0]>=0 and dx+bb[2]<=SIZE and dy+bb[1]>=0 and dy+bb[3]<=SIZE, (meta,scale,bb,dx,dy)
    canvas.alpha_composite(scaled,(dx,dy))
    return canvas, [px,py]

@lru_cache(maxsize=None)
def additions(name,rows,reference_indices):
    path=REVIEW/f'{name}.png'
    im=Image.open(path).convert('RGBA')
    frames=state_cells(im,rows)
    scale=BODY_HEIGHT/median(frames[i]['bounds'][3]-frames[i]['bounds'][1] for i in reference_indices)
    for f in frames:
        crop=cut(im,f)
        a=np.asarray(crop.getchannel('A'))
        l,t,r,b=f['bounds']
        yy,xx=np.nonzero(a[max(t,b-round(BODY_HEIGHT/scale*.16)):b]>64)
        f['pivotX']=float(np.median(xx)) if len(xx) else crop.width/2
    return path,im,frames,scale

def main():
    OUT.mkdir(parents=True,exist_ok=True)
    REVIEW.mkdir(parents=True,exist_ok=True)
    entries=json.loads((SOURCE/'animation-v1/atlas-manifest.json').read_text())['entries']
    selected=json.loads((SOURCE/'animation-v1/selected-atlases.json').read_text())
    records=[]
    sheets={}
    for d in DIRS:
        for state in ['walk','attack','states']:
            if state=='states':
                path=REVIEW/f'states-{d}.png';im=Image.open(path).convert('RGBA')
                frames=state_cells(im)
            else:
                filename=selected.get(f'{state}-{d}.png',f'{state}-{d}.png')
                entry=next(e for e in entries if e['file']==filename)
                path=SOURCE/'animation-v1'/filename;im=Image.open(path).convert('RGBA')
                frames=entry['frames']
            scale=BODY_HEIGHT/median(f['bounds'][3]-f['bounds'][1] for f in frames[:4])
            sheets[f'{state}-{d}']={'source':str(path.relative_to(ROOT)), 'scale':scale,'frames':frames}
            sequences={'walk':WALKS[d]} if state=='walk' else {'attack':ATTACKS[d]} if state=='attack' else {'idle':[0,1,2,3], 'hurt':([6,7,6,7] if d=='W' else [4,5,6,7]), 'death':list(range(8,16))}
            for action,indices in sequences.items():
                montage=Image.new('RGB',(SIZE*4, SIZE*((len(indices)+3)//4)),(27,32,29))
                draw=ImageDraw.Draw(montage)
                for i,source_index in enumerate(indices):
                    frame=frames[source_index]
                    use_path,use_im,use_frame,use_scale=path,im,frame,scale
                    if action=='attack' and d=='SW':
                        # Entire action is one coherent redraw, not a mixture of
                        # the bulky original and differently proportioned bridge.
                        # Neutral first/last poses establish scale; raised blades
                        # and leaning poses must never determine body size.
                        use_path,use_im,extra,use_scale=additions('attack-SW-v4',3,(0,11))
                        source_index=[0,1,2,3,4,5,6,7,9,10,11,0,0][i];use_frame=extra[source_index]
                    if action=='walk' and d=='S' and i>=4:
                        use_path,use_im,extra,use_scale=additions('walk-contact-S',1,(0,1,2,3))
                        source_index=i-4;use_frame=extra[source_index]
                    if action=='walk' and d in ('NE','NW','SW'):
                        use_path,use_im,extra,use_scale=additions(f'walk-polish-{d}',2,(0,1,2,3,4,5,6,7))
                        source_index=i;use_frame=extra[source_index]
                    use_scale*=DIRECTION_SCALE.get(d,1.0)
                    canvas,pivot=aligned(use_im,use_frame,use_scale)
                    name=f'Rustblade_{d}_{action}_{i:02d}'
                    canvas.save(OUT/f'{name}.png')
                    records.append({'name':name,'source':str(use_path.relative_to(ROOT)),'sourceFrame':source_index,'crop':use_frame,'scale':use_scale,'directionScale':DIRECTION_SCALE.get(d,1.0),'pivot':pivot,'root':[ROOT_X,ROOT_Y]})
                    ox,oy=(i%4)*SIZE,(i//4)*SIZE
                    montage.paste(canvas,(ox,oy),canvas)
                    draw.line((ox,oy+ROOT_Y,ox+SIZE,oy+ROOT_Y),fill=(78,100,78))
                    draw.text((ox+8,oy+8),f'{d} {action} {i} / source {source_index}',fill='white')
                montage.resize((768,montage.height//2),Image.Resampling.NEAREST).save(REVIEW/f'review-{d}-{action}.png')
    (REVIEW/'runtime-manifest.json').write_text(json.dumps(records,indent=2))
    (REVIEW/'extraction-metadata.json').write_text(json.dumps(sheets,indent=2))
    print(f'Prepared {len(records)} aligned sprites')

if __name__=='__main__': main()
