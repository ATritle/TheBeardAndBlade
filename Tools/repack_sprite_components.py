"""Lossless pose repack using measured connected silhouettes, never uniform slicing."""
from pathlib import Path
import argparse,json
from PIL import Image,ImageFilter
import numpy as np
p=argparse.ArgumentParser();p.add_argument('source');p.add_argument('output');p.add_argument('--columns',type=int,default=4);p.add_argument('--frames',type=int,default=12);p.add_argument('--order');p.add_argument('--source-rows');p.add_argument('--select');p.add_argument('--include-islands',action='store_true');p.add_argument('--min-body-area',type=int,default=1200);p.add_argument('--island-owner',action='append',default=[],help='Measured island x,y and owning body x,y, comma separated');a=p.parse_args()
im=Image.open(a.source).convert('RGBA');w,h=im.size
data=bytearray((np.array(im)[:,:,3]>64).astype('uint8').tobytes());components=[];islands=[]
for seed in range(w*h):
    if not data[seed]:continue
    data[seed]=0;stack=[seed];points=[];l=w;t=h;r=0;b=0
    while stack:
        q=stack.pop();y,x=divmod(q,w);points.append((x,y));l=min(l,x);r=max(r,x);t=min(t,y);b=max(b,y)
        for yy in range(max(0,y-1),min(h,y+2)):
            for xx in range(max(0,x-1),min(w,x+2)):
                n=yy*w+xx
                if data[n]:data[n]=0;stack.append(n)
    if len(points)>a.min_body_area:components.append((l,t,r+1,b+1,points))
    else:islands.append((l,t,r+1,b+1,points))
source_rows=list(map(int,a.source_rows.split(','))) if a.source_rows else None
expected=sum(source_rows) if source_rows else a.frames
if len(components)!=expected:raise SystemExit(f'Requires manual repair: {len(components)} components, expected {expected}; areas={sorted(len(c[4]) for c in components)}')
if a.include_islands:
    # Associate detached halo crystals with the nearest measured body, not grid slicing.
    original=list(components)
    for island in islands:
        il,it,ir,ib,points=island
        def distance(c):
            l,t,r,b,_=c
            return max(l-ir,il-r,0)**2+max(t-ib,it-b,0)**2
        index=min(range(len(original)),key=lambda i:distance(original[i]))
        for override in a.island_owner:
            ix,iy,tx,ty=map(int,override.split(','))
            if il<=ix<ir and it<=iy<ib:
                owners=[i for i,c in enumerate(original) if c[0]<=tx<c[2] and c[1]<=ty<c[3]]
                if len(owners)!=1:raise SystemExit('Ambiguous manual island owner')
                index=owners[0]
        l,t,r,b,body=components[index]
        components[index]=(min(l,il),min(t,it),max(r,ir),max(b,ib),body+points)
clipped=[i+1 for i,c in enumerate(components) if c[0]==0 or c[1]==0 or c[2]==w or c[3]==h]
if clipped:raise SystemExit(f'Requires source-edge repair before repacking: components {clipped}')
components.sort(key=lambda c:(c[1]+c[3])/2)
if source_rows:
    ordered=[];start=0
    for count in source_rows:
        ordered.extend(sorted(components[start:start+count],key=lambda c:c[0]));start+=count
    components=ordered
else:components=sum([sorted(components[i:i+a.columns],key=lambda c:c[0]) for i in range(0,len(components),a.columns)],[])
order=list(map(int,(a.select or a.order).split(','))) if (a.select or a.order) else list(range(a.frames))
assert len(order)==a.frames and len(set(order))==a.frames and all(0<=i<len(components) for i in order)
cw=max(c[2]-c[0] for c in components)+96;ch=max(c[3]-c[1] for c in components)+96
out=Image.new('RGBA',(cw*a.columns,ch*((a.frames+a.columns-1)//a.columns)))
records=[]
for i,n in enumerate(order):
    l,t,r,b,points=components[n];mask=Image.new('L',im.size)
    pixels=mask.load()
    for x,y in points:pixels[x,y]=255
    mask=mask.filter(ImageFilter.MaxFilter(5));arr=np.array(im);keep=np.array(mask)>0;arr[~keep]=0;arr[arr[:,:,3]==0,:3]=0
    sprite=Image.fromarray(arr).crop((max(0,l-2),max(0,t-2),min(w,r+2),min(h,b+2)))
    x=(i%a.columns)*cw+(cw-sprite.width)//2;y=(i//a.columns)*ch+ch-48-sprite.height
    out.alpha_composite(sprite,(x,y));records.append({'sourcePose':n+1,'sourceBounds':[l,t,r,b],'destination':[x,y],'cell':[x,y,sprite.width,sprite.height],'height':b-t})
out.save(a.output)
Path(a.output+'.repack.json').write_text(json.dumps({'source':a.source,'pixelResampling':False,'includesDetachedIslands':a.include_islands,'operation':'Measured silhouette masks; 48px margins; bottom-center anchors','frames':records},indent=2)+'\n')
print(json.dumps({'output':a.output,'poses':len(records),'heights':[r['height'] for r in records]}))
