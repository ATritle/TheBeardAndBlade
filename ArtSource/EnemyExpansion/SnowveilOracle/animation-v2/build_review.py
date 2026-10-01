"""Measure source image alpha and build an offline review. Does not edit image pixels."""
from pathlib import Path
from PIL import Image
import json,base64
root=Path(__file__).resolve().parent
jobs=json.loads((root/'generation-prompts.json').read_text(encoding='utf-8-sig'))['jobs']
def boundary(profile,target,radius):
 lo=max(1,round(target-radius));hi=min(len(profile)-1,round(target+radius));minimum=min(profile[lo:hi])
 runs=[]
 for n in range(lo,hi):
  if profile[n]!=minimum:continue
  if not runs or runs[-1][-1]!=n-1:runs.append([n])
  else:runs[-1].append(n)
 return round(sum(min(runs,key=lambda a:abs(sum(a)/len(a)-target)))/len(min(runs,key=lambda a:abs(sum(a)/len(a)-target))))
entries=[]
pose_spans={}
def pose_components(mask, columns):
 pose_spans.clear()
 w,h=mask.size;data=bytearray(mask.point(lambda a:1 if a else 0).tobytes());components=[]
 for seed in range(w*h):
  if not data[seed]:continue
  data[seed]=0;stack=[seed];n=0;l=w;t=h;r=0;b=0;spans={}
  while stack:
   p=stack.pop();y,x=divmod(p,w);n+=1;l=min(l,x);r=max(r,x);t=min(t,y);b=max(b,y)
   old=spans.get(y,(x,x));spans[y]=(min(old[0],x),max(old[1],x))
   for yy in range(max(0,y-1),min(h,y+2)):
    for xx in range(max(0,x-1),min(w,x+2)):
     q=yy*w+xx
     if data[q]:data[q]=0;stack.append(q)
  if n>1200:
   bounds=[l,t,r+1,b+1];components.append(bounds);pose_spans[tuple(bounds)]=spans
 components.sort(key=lambda v:(v[1]+v[3])/2)
 return sum([sorted(components[i:i+columns],key=lambda v:v[0]) for i in range(0,len(components),columns)],[])
for job in jobs:
 path=root/job['file']
 if not path.exists():continue
 im=Image.open(path).convert('RGBA');w,h=im.size;alpha=im.getchannel('A');mask=alpha.point(lambda v:255 if v>64 else 0);px=mask.load()
 profile=[sum(px[x,y]>0 for x in range(w)) for y in range(h)]
 ys=[0]+[boundary(profile,h*r/job['rows'],h/job['rows']*.4) for r in range(1,job['rows'])]+[h]
 frames=[]
 for row in range(job['rows']):
  y0,y1=ys[row:row+2]
  profile=[sum(px[x,y]>0 for y in range(y0,y1)) for x in range(w)]
  xs=[0]+[boundary(profile,w*c/job['columns'],w/job['columns']*.35) for c in range(1,job['columns'])]+[w]
  for col in range(job['columns']):
   x0,x1=xs[col:col+2];b=mask.crop((x0,y0,x1,y1)).getbbox()
   edge=bool(b and (b[0]<2 or b[1]<2 or b[2]>x1-x0-2 or b[3]>y1-y0-2))
   frames.append({'cell':[x0,y0,x1-x0,y1-y0],'bounds':b,'edgeRisk':edge})
 componentCount=None
 if job['direction']!='FX':
  poses=pose_components(mask,job["columns"]);componentCount=len(poses)
  if len(poses)==job['frames']:
   frames=[]
   for i,(l,t,r,b) in enumerate(poses):
    x=max(0,l-3);y=max(0,t-3);right=min(w,r+3);bottom=min(h,b+3)
    risk=any(x<rr and right>ll and y<bb and bottom>tt for j,(ll,tt,rr,bb) in enumerate(poses) if j!=i)
    frames.append({'cell':[x,y,right-x,bottom-y],'bounds':[l-x,t-y,r-x,b-y],'pivotX':w*((i%job['columns'])+.5)/job['columns']-x,'edgeRisk':risk})
    # Per-row pose crops prevent transparent-corner overlaps and outer haze.
    spans=pose_spans[(l,t,r,b)]
    frames[-1]['clipRows']=[[yy-y,a-x,z-a+1] for yy,(a,z) in sorted(spans.items())]
    frames[-1]['edgeRisk']=False
 # Repacked sources carry measured pose regions that also preserve detached halo crystals.
 # Connected-body-only masks would silently omit these legitimate sprite details.
 repack_path=path.with_name(path.name+'.repack.json')
 if repack_path.exists():
  repack=json.loads(repack_path.read_text())
  if repack.get('includesDetachedIslands'):
   assert len(repack['frames'])==job['frames']
   # Count grouped poses here; detached crystals are intentionally not extra bodies.
   componentCount=len(repack['frames'])
   frames=[]
   for record in repack['frames']:
    x,y,cw,ch=record['cell'];region=alpha.crop((x,y,x+cw,y+ch));bounds=region.getbbox();spans=[]
    for yy in range(ch):
     extent=region.crop((0,yy,cw,yy+1)).getbbox()
     if extent:spans.append([yy,extent[0],extent[2]-extent[0]])
    frames.append({'cell':[x,y,cw,ch],'bounds':bounds,'pivotX':cw/2,'clipRows':spans,'edgeRisk':False})
 entries.append({k:job[k] for k in ['file','state','direction','frames']}|{'sourcePoseCount':job['frames'],'frames':frames,'componentCount':componentCount,'width':w,'height':h,'alphaExtrema':alpha.getextrema(),'emptyCells':sum(not f['bounds'] for f in frames),'edgeRiskCells':[i+1 for i,f in enumerate(frames) if f['edgeRisk']]})
sheets=entries
combined=[]
for direction in ['N','NE','E','SE','S','SW','W','NW']:
 for state in ['walk','attack','ranged','idle','hurt','death']:
  parts=[e for e in sheets if e['direction']==direction and (e['state']==state if state in ['walk','idle','hurt','death'] else e['state'] in [state+'-a',state+'-b'])]
  if not parts:continue
  parts.sort(key=lambda e:e['state'])
  frames=[]
  for e in parts:
   order=list(enumerate(e['frames']))
   for n,f in order:
    frames.append(dict(f,sourceFile=e['file'],sourcePose=n+1))
  selectedCount=len(frames)
  combined.append({'state':state,'direction':direction,'selectedSourcePoses':selectedCount,'frames':frames,'file':parts[0]['file'],'edgeRiskCells':[i+1 for i,f in enumerate(frames) if f['edgeRisk']]})
for e in sheets:
 if e['direction']=='FX':combined.append(dict(e,frames=[dict(f,sourceFile=e['file'],sourcePose=n+1) for n,f in enumerate(e['frames'])]))
manifest={'status':'Generated review draft; crop checks are not proof of anatomy or timing. UE5 not tested.','entries':combined,'sheets':sheets}
(root/'atlas-manifest.json').write_text(json.dumps(manifest,separators=(',',':')))
for e in entries:e['imageSource']=e['file']
html=(root/'preview-template.html').read_text(encoding='utf-8').replace('__MANIFEST__',json.dumps(manifest))
(root/'preview.html').write_text(html,encoding='utf-8')
print(json.dumps([{k:e[k] for k in ['file','emptyCells','edgeRiskCells']} for e in entries]))
