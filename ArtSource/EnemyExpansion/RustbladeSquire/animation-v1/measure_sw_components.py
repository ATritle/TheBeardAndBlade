"""Read alpha components to measure pose rectangles; no image pixels are edited."""
from pathlib import Path
from PIL import Image
import json
root=Path(__file__).resolve().parent
results={}
for name in ['walk-SW-right-hand-v3.png','attack-SW-right-hand-v5.png']:
    im=Image.open(root/name).convert('RGBA');w,h=im.size
    alpha=im.getchannel('A'); data=bytearray(alpha.point(lambda a: 1 if a>64 else 0).tobytes())
    components=[]
    for seed in range(w*h):
        if not data[seed]:continue
        data[seed]=0;stack=[seed];count=0;x0=w;y0=h;x1=0;y1=0
        while stack:
            p=stack.pop();y,x=divmod(p,w);count+=1
            x0=min(x0,x);x1=max(x1,x);y0=min(y0,y);y1=max(y1,y)
            for yy in range(max(0,y-1),min(h,y+2)):
                for xx in range(max(0,x-1),min(w,x+2)):
                    q=yy*w+xx
                    if data[q]:data[q]=0;stack.append(q)
        if count>1000:components.append({'bounds':[x0,y0,x1+1,y1+1],'pixels':count})
    components.sort(key=lambda c:(c['bounds'][1]+c['bounds'][3])/2)
    ordered=[]
    for start in range(0,len(components),4):ordered+=sorted(components[start:start+4],key=lambda c:c['bounds'][0])
    results[name]=ordered
(root/'sw-pose-bounds.json').write_text(json.dumps(results,indent=2))
print(json.dumps(results))
