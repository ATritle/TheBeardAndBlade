"""Register two corrected SW sheets; retain generated originals and measured grips.

Local crop/alignment processing only. No generated/painted replacement pixels.
"""
from pathlib import Path
import json, shutil
from PIL import Image, ImageFilter

root=Path(__file__).resolve().parents[1]
out=root/'ArtSource/HeroMeleeRigV1';out.mkdir(parents=True,exist_ok=True)
source=Path(r'C:/Users/tritl/.codex/generated_images/01a0bffb-e891-7110-bda7-4f3879f2880b')
specs=[
 ('MeleeIdle_SW','exec-3bf9c698-9f1e-4932-bf99-d207f1887bc8.png',
  [(126,213),(129,213),(128,213),(126,213),(126,214),(128,214),(126,214),(128,214)],
  [204,204,204,204,204,204,204,204]),
 ('MeleeBlock_SW','exec-9440cb91-a79d-4de1-9627-0ad3565f41ad.png',
  [(136,225),(174,129),(149,106),(149,106),(149,107),(231,141),(149,108),(136,225)],
  [213,213,213,213,213,231,213,213])
]
records={}
for name,filename,hands,roots in specs:
    dest=out/(name+'_source.png')
    if not dest.exists():shutil.copy2(source/filename,dest)
    sheet=Image.open(dest).convert('RGBA');w,h=sheet.size
    # Remove only saturated red matte contamination at the silhouette boundary.
    # Original RGBA stays alongside the registered export for reversibility.
    alpha=sheet.getchannel('A');inner=alpha.point(lambda a:255 if a>32 else 0).filter(ImageFilter.MinFilter(5))
    pixels=sheet.load();inside=inner.load();removed=0
    for py in range(h):
        for px in range(w):
            r,g,b,a=pixels[px,py]
            if a and not inside[px,py] and r>120 and g<70 and b<70 and r>g*2.5:
                pixels[px,py]=(r,g,b,0);removed+=1
    print(name,'edge matte pixels removed:',removed)
    cells=[]
    for f in range(8):
        rect=(round(f%4*w/4),round(f//4*h/2),round((f%4+1)*w/4),round((f//4+1)*h/2))
        im=sheet.crop(rect)
        bbox=im.getchannel('A').point(lambda a:255 if a>=32 else 0).getbbox()
        assert bbox and bbox[0]>2 and bbox[2]<im.width-2,(name,f,bbox)
        cells.append((im,bbox))
    # Uniform scale within each action, not per-frame silhouette scaling.
    scale=220/max(b[3]-b[1] for _,b in cells)
    atlas=Image.new('RGBA',(1024,512));points=[]
    for f,(im,bbox) in enumerate(cells):
        crop=im.crop(bbox)
        resized=crop.resize((round(crop.width*scale),round(crop.height*scale)),Image.Resampling.LANCZOS)
        x=round(128-(roots[f]-bbox[0])*scale);y=232-resized.height
        assert x>=0 and x+resized.width<=256,(name,f,x,resized.width)
        atlas.alpha_composite(resized,(f%4*256+x,f//4*256+y))
        points.append([round(x+(hands[f][0]-bbox[0])*scale,2),round(y+(hands[f][1]-bbox[1])*scale,2)])
    atlas.save(out/(name+'.png'));records[name]=points
(out/'registration.json').write_text(json.dumps(records,indent=2)+'\n')
print(json.dumps(records,indent=2))
