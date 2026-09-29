"""Bake supplied measured pose masks; never uniform-slice the source packs."""
import json
import argparse
from pathlib import Path
from functools import lru_cache
from statistics import median
import numpy as np
from PIL import Image, ImageDraw
from prepare_rustblade import state_cells

ROOT = Path(__file__).resolve().parents[1]
NAMES = ['GraveglassSlinger', 'ChainboundBailiff', 'CandleHexer', 'SepulcherLancer']
DIRS = ['N','NE','E','SE','S','SW','W','NW']
SIZE, RX, RY = 512, 256, 450

@lru_cache(None)
def image(path):
    return Image.open(path).convert('RGBA')

def cut(path, frame):
    x,y,w,h = frame['cell']
    im = image(str(path)).crop((x,y,x+w,y+h))
    alpha = np.array(im.getchannel('A'))
    alpha[alpha < 32] = 0
    mask = np.zeros_like(alpha)
    if 'clipRows' in frame:
        # Supplied schema is LOCAL Y, LOCAL X, WIDTH (not right endpoint).
        for yy,xx,width in frame['clipRows']:
            mask[yy,xx:xx+width] = 255
    else:
        l,t,r,b = frame['bounds']
        mask[t:b,l:r] = 255
    alpha[mask == 0] = 0
    im.putalpha(Image.fromarray(alpha))
    assert im.getbbox(), (path,frame['cell'])
    return im

def bake(im, frame, scale):
    bb = im.getbbox()
    px,py = frame.get('pivotX',im.width/2),bb[3]-1
    im = im.resize((round(im.width*scale),round(im.height*scale)),Image.Resampling.NEAREST)
    dx,dy = round(RX-px*scale),round(RY-py*scale)
    bb = im.getbbox()
    assert dx+bb[0]>=0 and dx+bb[2]<=SIZE and dy+bb[1]>=0, (bb,dx,dy,scale)
    canvas=Image.new('RGBA',(SIZE,SIZE));canvas.alpha_composite(im,(dx,dy))
    return canvas

def body_height(im,frame):
    """Neutral anatomy calibration excludes narrow spears/flames above the head."""
    a=np.asarray(im);bb=im.getbbox();h=bb[3]-bb[1]
    px=frame.get('pivotX',im.width/2)
    lo,hi=max(0,round(px-h*.15)),min(im.width,round(px+h*.15))
    solid=(a[:,:,3]>128)&(a[:,:,:3].mean(axis=2)<185)
    counts=solid[:,lo:hi].sum(axis=1)
    hits=counts>max(8,h*.055)
    run=np.convolve(hits.astype(int),np.ones(4,dtype=int),mode='valid')
    ys=np.flatnonzero(run==4)
    top=int(ys[0]) if len(ys) else bb[1]
    return bb[3]-top

def export():
    for name in NAMES:
        src=ROOT/'ArtSource/EnemyExpansion'/name/'animation-v1'
        review=src.parent/'runtime-v1';out=ROOT/'Content/Art/EnemyExpansion'/name;out.mkdir(parents=True,exist_ok=True)
        entries=json.loads((src/'atlas-manifest.json').read_text())['entries'];records=[]
        for e in entries:
            if e['direction']=='FX':
                # FX cells preserve detached fragments. Center in one fixed canvas,
                # with one shared scale, not a fit-to-frame that pumps the explosion.
                frames=e['frames'];ims=[cut(src/f.get('sourceFile',e['file']),f) for f in frames]
                maxdim=max(max(im.width,im.height) for im in ims);scale=240/maxdim
                for i,(im,f) in enumerate(zip(ims,frames)):
                    im=im.resize((round(im.width*scale),round(im.height*scale)),Image.Resampling.NEAREST)
                    canvas=Image.new('RGBA',(256,256));canvas.alpha_composite(im,((256-im.width)//2,(256-im.height)//2))
                    asset=f'{name}_FX_{e["state"]}_{i:02d}';canvas.save(out/(asset+'.png'))
                    records.append({'name':asset,'source':str((src/f.get('sourceFile',e['file'])).relative_to(ROOT)),'crop':f,'scale':scale})
        for d in DIRS:
            for action in ['walk','attack','states']:
                if action=='states':
                    path=review/f'states-{d}-v2.png'
                    if not path.exists():path=review/f'states-{d}.png'
                    im=image(str(path));frames=state_cells(im)
                    for fi,f in enumerate(frames):
                        crop=cut(path,f);bb=crop.getbbox();a=np.asarray(crop.getchannel('A'))
                        _,xx=np.nonzero(a[max(bb[1],bb[3]-round((bb[3]-bb[1])*.13)):bb[3]]>64)
                        f['pivotX']=float(np.median(xx)) if len(xx) else crop.width/2
                        if fi>=12:f['pivotX']=(bb[0]+bb[2])/2
                        f['sourceFile']=path.name
                    base=review;entry={'file':path.name};seqs={'idle':list(range(4)),'hurt':list(range(4,8)),'death':list(range(8,16))}
                    if name=='GraveglassSlinger' and d=='N':seqs['idle']=[0,1,3,1]
                    if (name,d) in [('GraveglassSlinger','SW'),('ChainboundBailiff','NW'),('CandleHexer','NW'),('CandleHexer','NE'),('CandleHexer','SW')]:seqs['hurt']=[4,6,6,7]
                    if name=='CandleHexer' and d=='E':seqs['hurt']=[5,6,6,7]
                    if name=='SepulcherLancer' and d=='NE':seqs['idle']=[1,3,1,3]
                    if name=='SepulcherLancer' and d=='SE':seqs['idle']=[0,1,3,1]
                    if name=='GraveglassSlinger' and d=='SE':seqs['hurt']=[4,6,6,7]
                    if name=='GraveglassSlinger' and d=='S':seqs['death']=[9,9,10,11,12,13,14,15]
                else:
                    entry=next(e for e in entries if e['state']==action and e['direction']==d)
                    frames=entry['frames'];base=src
                    if action=='walk':seqs={'walk':[0,2,4,6,8,10,12,14]}
                    else:
                        # Curated removal of repeated ready holds. These are indices
                        # into the already corrected manifest, never rejected sheets.
                        order=[0,2,3,4,5,6,7,8,9,10,12,14,16,18,21,23]
                        if name=='GraveglassSlinger':order=[0,2,3,4,5,7,8,9,10,11,12,14,16,18,21,23]
                        if name=='GraveglassSlinger' and d=='SW':order=[0,2,4,5,6,8,9,10,11,12,13,14,16,18,21,23]
                        # NW loading/late holds swap hands in the supplied art.
                        # Keep the far/right-hand ready, windup and release poses.
                        if name=='GraveglassSlinger' and d=='NW':order=[4,4,5,5,6,6,7,7,10,11,13,14,15,4,4,4]
                        if name=='CandleHexer':order=[0,1,2,3,4,5,6,7,8,9,10,12,14,16,20,23]
                        if name=='SepulcherLancer':order=[0,1,2,3,4,5,6,7,8,9,10,12,15,18,21,23]
                        if name=='ChainboundBailiff' and d=='NW':order=[0,2,3,4,5,6,7,8,10,11,12,14,16,18,21,23]
                        if name=='CandleHexer' and d=='W':order=[0,1,2,3,4,5,6,8,10,11,12,14,16,18,21,23]
                        if name=='SepulcherLancer' and d=='N':order=[0,2,4,5,6,7,8,11,12,13,14,16,18,20,22,23]
                        seqs={'attack':order}
                scales={}
                for f in frames:
                    file=f.get('sourceFile',entry['file'])
                    if file not in scales:
                        refs=[g for g in frames if g.get('sourceFile',entry['file'])==file]
                        refs=refs[-3:] if file.startswith('attack-b') else refs[:3]
                        if action=='states':refs=frames[:3]
                        if name=='ChainboundBailiff' and d=='NW' and file=='attack-a-NW-right.png':refs=[frames[0]]
                        scales[file]=205/median(body_height(cut(base/file,g),g) for g in refs)
                for state,indices in seqs.items():
                    board=Image.new('RGB',(256*4,256*((len(indices)+3)//4)),(27,32,29));draw=ImageDraw.Draw(board)
                    for i,j in enumerate(indices):
                        f=frames[j];file=f.get('sourceFile',entry['file']);scale=scales[file]
                        canvas=bake(cut(base/file,f),f,scale);asset=f'{name}_{d}_{state}_{i:02d}';canvas.save(out/(asset+'.png'))
                        records.append({'name':asset,'source':str((base/file).relative_to(ROOT)),'sourceFrame':j,'crop':f,'scale':scale,'root':[RX,RY]})
                        thumb=canvas.resize((256,256),Image.Resampling.NEAREST);ox,oy=(i%4)*256,(i//4)*256;board.paste(thumb,(ox,oy),thumb)
                        draw.line((ox,oy+RY/2,ox+256,oy+RY/2),fill=(60,80,60));draw.text((ox+5,oy+5),f'{d} {state} {i} / {j}',fill='white')
                    board.save(review/f'review-{d}-{state}.png')
        (review/'runtime-manifest.json').write_text(json.dumps(records,indent=2));print(name,len(records),'frames prepared')

def main():
    for name in NAMES:
        src=ROOT/'ArtSource/EnemyExpansion'/name/'animation-v1'
        review=src.parent/'runtime-v1';review.mkdir(exist_ok=True)
        entries=json.loads((src/'atlas-manifest.json').read_text())['entries']
        summary=[]
        for entry in entries:
            state,d=entry['state'],entry['direction']
            frames=entry['frames']
            if d=='FX':continue
            scales={}
            for f in frames:
                file=f.get('sourceFile',entry['file'])
                if file not in scales:
                    refs=[g for g in frames if g.get('sourceFile',entry['file'])==file][:3]
                    scales[file]=205/median(cut(src/file,g).getbbox()[3]-cut(src/file,g).getbbox()[1] for g in refs)
            board=Image.new('RGB',(192*8,210*((len(frames)+7)//8)),(30,32,33));draw=ImageDraw.Draw(board)
            for i,f in enumerate(frames):
                file=f.get('sourceFile',entry['file']);im=cut(src/file,f)
                canvas=bake(im,f,scales[file])
                ox,oy=i%8*192,i//8*210
                thumb=canvas.resize((192,192),Image.Resampling.NEAREST)
                board.paste(thumb,(ox,oy),thumb);draw.text((ox+5,oy+192),f'{i}: {file} #{f.get("sourcePose",i+1)}',fill='white')
            board.save(review/f'source-review-{d}-{state}.png')
            summary.append({'state':state,'direction':d,'frames':len(frames),'files':scales,'selection':[(f.get('sourceFile',entry['file']),f.get('sourcePose',i+1)) for i,f in enumerate(frames)]})
        (review/'source-summary.json').write_text(json.dumps(summary,indent=2))
        print(name,'review boards ready')

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--export',action='store_true');args=parser.parse_args()
    export() if args.export else main()
