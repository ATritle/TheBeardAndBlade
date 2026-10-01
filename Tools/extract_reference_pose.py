"""Extract a measured, masked pose for image-generation identity reference."""
import argparse,json
from pathlib import Path
from PIL import Image,ImageDraw,ImageFilter
p=argparse.ArgumentParser();p.add_argument('manifest');p.add_argument('sheet');p.add_argument('output');p.add_argument('--pose',type=int,default=1);a=p.parse_args()
root=Path(a.manifest).parent
m=json.loads(Path(a.manifest).read_text())
sheet=next(s for s in m['sheets'] if s['file']==a.sheet)
f=sheet['frames'][a.pose-1];x,y,w,h=f['cell']
im=Image.open(root/a.sheet).convert('RGBA').crop((x,y,x+w,y+h))
if f.get('clipRows'):
    mask=Image.new('L',im.size);d=ImageDraw.Draw(mask)
    for yy,xx,ww in f['clipRows']:d.line((xx,yy,xx+ww-1,yy),fill=255)
    mask=mask.filter(ImageFilter.MaxFilter(5))
    from PIL import ImageChops
    im.putalpha(ImageChops.multiply(im.getchannel('A'),mask))
canvas=Image.new('RGBA',(w+96,h+96));canvas.alpha_composite(im,(48,48));canvas.save(a.output)
