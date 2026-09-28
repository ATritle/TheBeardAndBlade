"""Measure existing atlas alpha and write crop metadata. Does not alter images."""
from pathlib import Path
import json
from PIL import Image
ROOT=Path(__file__).resolve().parent
jobs=json.loads((ROOT/'generation-prompts.json').read_text())['jobs']
selected=json.loads((ROOT/'selected-atlases.json').read_text()) if (ROOT/'selected-atlases.json').exists() else {}
def gutter(profile, target, radius):
    lo=max(1,round(target-radius));hi=min(len(profile)-1,round(target+radius))
    best=min(profile[lo:hi]);points=[i for i in range(lo,hi) if profile[i]==best]
    runs=[]
    for p in points:
        if not runs or p!=runs[-1][-1]+1:runs.append([p])
        else:runs[-1].append(p)
    run=min(runs,key=lambda r:abs(sum(r)/len(r)-target))
    return round(sum(run)/len(run))
entries=[]
for job in jobs:
    filename=selected.get(job['file'],job['file'])
    path=ROOT/filename
    if not path.exists():continue
    im=Image.open(path).convert('RGBA');alpha=im.getchannel('A');w,h=im.size
    mask=alpha.point(lambda v:255 if v>64 else 0);px=mask.load()
    ys=[0]+[gutter([sum(px[x,y]>0 for x in range(w)) for y in range(h)],h*r/job['rows'],h/job['rows']*.13) for r in range(1,job['rows'])]+[h]
    frames=[]
    for row in range(job['rows']):
        y0,y1=ys[row:row+2]
        profile=[sum(px[x,y]>0 for y in range(y0,y1)) for x in range(w)]
        xs=[0]+[gutter(profile,w*c/job['columns'],w/job['columns']*.13) for c in range(1,job['columns'])]+[w]
        for col in range(job['columns']):
            x0,x1=xs[col:col+2];bounds=mask.crop((x0,y0,x1,y1)).getbbox()
            frames.append({'cell':[x0,y0,x1-x0,y1-y0],'bounds':list(bounds) if bounds else None,'edgeRisk':bool(bounds and (bounds[0]<=1 or bounds[1]<=1 or bounds[2]>=x1-x0-1 or bounds[3]>=y1-y0-1))})
    if filename in ['attack-SW-right-hand-v5.png','attack-NW-right-hand-v4.png']:
        # Revised sheets have uneven row spacing. Locate gutters per column;
        # do not slice a raised blade using a neighboring column's foot line.
        frames=[None]*(job['rows']*job['columns'])
        for col in range(job['columns']):
            x0=round(w*col/job['columns']);x1=round(w*(col+1)/job['columns'])
            profile=[sum(px[x,y]>0 for x in range(x0,x1)) for y in range(h)]
            targets=([240,458,714,930,1160] if job['direction']=='SW' else [210,410,640,854,1060])
            cuts=[0]+[gutter(profile,t,55) for t in targets]+[h]
            for row in range(job['rows']):
                y0,y1=cuts[row:row+2];bounds=mask.crop((x0,y0,x1,y1)).getbbox()
                frames[row*job['columns']+col]={'cell':[x0,y0,x1-x0,y1-y0],'bounds':list(bounds) if bounds else None,'edgeRisk':bool(bounds and (bounds[0]<=1 or bounds[1]<=1 or bounds[2]>=x1-x0-1 or bounds[3]>=y1-y0-1))}
    if filename in ['walk-SW-right-hand-v3.png','attack-SW-right-hand-v5.png']:
        poses=json.loads((ROOT/'sw-pose-bounds.json').read_text())[filename]
        assert len(poses)==job['frames'], 'SW pose inventory mismatch'
        frames=[]
        for index,pose in enumerate(poses):
            l,t,r,b=pose['bounds'];x0=max(0,l-4);y0=max(0,t-4);x1=min(w,r+4);y1=min(h,b+4)
            # Preserve the old atlas column anchor while excluding adjacent poses.
            frames.append({'cell':[x0,y0,x1-x0,y1-y0],'bounds':[l-x0,t-y0,r-x0,b-y0],'pivotX':w*((index%4)+.5)/4-x0,'edgeRisk':False})
    entries.append({'file':filename,'direction':job['direction'],'state':job['state'],'width':w,'height':h,'alphaExtrema':alpha.getextrema(),'frames':frames,'expectedFrames':job['frames'],'emptyCells':sum(f['bounds'] is None for f in frames),'edgeRiskCells':[i for i,f in enumerate(frames) if f['edgeRisk']]})
report={'status':'Generated draft atlases. Measured cells are not proof of unique or anatomically correct poses. UE5 review pending.','entries':entries}
(ROOT/'atlas-manifest.json').write_text(json.dumps(report,indent=2))
import runpy
runpy.run_path(str(ROOT/'build_preview.py'))
print(json.dumps([{'file':e['file'],'size':[e['width'],e['height']],'frames':len(e['frames']),'emptyCells':e['emptyCells'],'edgeRiskCells':e['edgeRiskCells']} for e in entries],indent=2))
