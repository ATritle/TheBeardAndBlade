"""Extract a measured repack cell as an animation reference, without resampling."""
import argparse,json
from pathlib import Path
from PIL import Image
p=argparse.ArgumentParser();p.add_argument('sheet');p.add_argument('output');p.add_argument('--pose',type=int,default=12);a=p.parse_args()
record=json.loads(Path(a.sheet+'.repack.json').read_text())['frames'][a.pose-1]
x,y,w,h=record['cell'];im=Image.open(a.sheet).convert('RGBA').crop((x,y,x+w,y+h))
out=Image.new('RGBA',(w+192,h+192));out.alpha_composite(im,(96,96));out.save(a.output)
