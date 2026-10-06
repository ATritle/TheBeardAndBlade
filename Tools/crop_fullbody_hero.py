"""Measure connected sprite silhouettes; do not uniformly slice neighboring art.

Provisional crops only. No skeletal deformation, new poses, hand swapping,
rescaling, engine imports or gameplay changes. Original PNGs remain untouched.
"""
from pathlib import Path
import json, argparse
import numpy as np
from PIL import Image, ImageFilter

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'ArtSource/HeroFullBodyV1'
spec=json.loads((OUT/'production-spec.json').read_text())
if (OUT/'additional-spec.json').exists():spec['clips']+=json.loads((OUT/'additional-spec.json').read_text())['clips']
selections=json.loads((OUT/'selections.json').read_text()) if (OUT/'selections.json').exists() else {}
p=argparse.ArgumentParser();p.add_argument('--name');args=p.parse_args()

def components(mask):
    parents=[];runs=[];previous=[]
    def find(i):
        while parents[i]!=i:
            parents[i]=parents[parents[i]];i=parents[i]
        return i
    def join(a,b):
        a,b=find(a),find(b)
        if a!=b:parents[b]=a
    for y,row in enumerate(mask):
        changes=np.flatnonzero(np.diff(np.r_[False,row,False].astype(np.int8)))
        current=[];j=0
        for x0,x1 in changes.reshape(-1,2):
            i=len(parents);parents.append(i);runs.append((y,int(x0),int(x1),i));current.append((int(x0),int(x1),i))
            while j<len(previous) and previous[j][1]<x0-1:j+=1
            k=j
            while k<len(previous) and previous[k][0]<=x1+1:
                join(i,previous[k][2]);k+=1
        previous=current
    groups={}
    for y,x0,x1,i in runs:
        key=find(i)
        g=groups.setdefault(key,{'area':0,'box':[x0,y,x1,y+1],'runs':[]})
        g['area']+=x1-x0;b=g['box']
        b[0]=min(b[0],x0);b[1]=min(b[1],y);b[2]=max(b[2],x1);b[3]=max(b[3],y+1)
        g['runs'].append((y,x0,x1))
    return groups

all_results=[]
for clip in spec['clips']:
    for direction in range(8):
        name=f"{clip['name']}_{direction}"
        if args.name and name!=args.name:continue
        path=OUT/selections.get(name, f'production/{name}.png')
        if not path.exists():continue
        image=Image.open(path).convert('RGBA');pixels=np.array(image)
        # Inspected SE slash has a complete fist four pixels below the edge.
        # Remove only its faint exterior fringe in derived crops; retain source.
        alpha_floor=40 if name=='MeleeSlash_3' else 12
        pixels[pixels[:,:,3]<alpha_floor]=0
        h,w=pixels.shape[:2];groups=components(pixels[:,:,3]>40)
        candidates={i:[] for i in range(8)}
        for key,g in groups.items():
            if g['area']<300:continue
            x0,y0,x1,y1=g['box'];cx=(x0+x1)/2;cy=(y0+y1)/2
            cell=min(3,int(cx/(w/4)))+4*min(1,int(cy/(h/2)))
            candidates[cell].append(key)
        source={'name':name,'path':str(path.relative_to(ROOT)),'size':[w,h],
                'status':'provisional-crops','alphaFloor':alpha_floor,
                'manualAnchorReview':False,'frames':[]}
        target=OUT/'crops'/name;target.mkdir(parents=True,exist_ok=True)
        for index in range(8):
            keys=sorted(candidates[index],key=lambda k:groups[k]['area'],reverse=True)
            if not keys:
                source['frames'].append({'index':index,'error':'No complete figure detected'});continue
            main=groups[keys[0]];box=main['box'];selected=[keys[0]]
            for key,g in groups.items():
                if key==keys[0] or g['area']>main['area']*.15:continue
                b=g['box'];cx=(b[0]+b[2])/2;cy=(b[1]+b[3])/2
                if box[0]-10<=cx<=box[2]+10 and box[1]-10<=cy<=box[3]+10:selected.append(key)
            mask=np.zeros((h,w),dtype=np.uint8)
            for key in selected:
                for y,x0,x1 in groups[key]['runs']:mask[y,x0:x1]=255
            keep=np.asarray(Image.fromarray(mask).filter(ImageFilter.MaxFilter(7)))>0
            cleaned=pixels.copy();cleaned[:,:,3]=np.where(keep,pixels[:,:,3],0)
            cleaned[cleaned[:,:,3]<12]=0
            result=Image.fromarray(cleaned);bounds=result.getbbox()
            if not bounds:
                source['frames'].append({'index':index,'error':'Empty extracted figure'});continue
            result.crop(bounds).save(target/f'{index:02}.png')
            flags=[]
            if len(keys)>1 and groups[keys[1]]['area']>main['area']*.2:flags.append('multiple large components; inspect missing/neighbor parts')
            if bounds[0]==0 or bounds[1]==0 or bounds[2]==w or bounds[3]==h:flags.append('touches source edge; inspect clipping')
            source['frames'].append({'index':index,'measuredRect':list(bounds),
                                     'mainArea':main['area'],'flags':flags,
                                     'root':None,'rightHand':None,'leftHand':None,
                                     'weaponAngle':None,'bowMuzzle':None,
                                     'visualApproval':False})
        (target/'registration.json').write_text(json.dumps(source,indent=2))
        all_results.append({'name':name,'frames':sum('error' not in f for f in source['frames']),
                            'flags':sum(bool(f.get('flags') or f.get('error')) for f in source['frames'])})
print(json.dumps(all_results))
