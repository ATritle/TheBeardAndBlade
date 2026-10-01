"""Replace a measured pose with a reviewed repair, retaining its ground anchor."""
import argparse,json
from pathlib import Path
from PIL import Image
p=argparse.ArgumentParser();p.add_argument('sheet');p.add_argument('repair');p.add_argument('output');p.add_argument('--pose',type=int,required=True);p.add_argument('--height',type=int,help='Reviewed body-scale correction when the old pose height is not a valid reference');a=p.parse_args()
im=Image.open(a.sheet).convert('RGBA');meta=json.loads(Path(a.sheet+'.repack.json').read_text());r=meta['frames'][a.pose-1]
x,y,w,h=r['cell'];old=im.crop((x,y,x+w,y+h));box=old.getchannel('A').point(lambda v:255 if v>64 else 0).getbbox()
repair=Image.open(a.repair).convert('RGBA');bounds=repair.getchannel('A').point(lambda v:255 if v>64 else 0).getbbox();repair=repair.crop(bounds)
height=a.height or box[3]-box[1];scale=height/repair.height;repair=repair.resize((round(repair.width*scale),height),Image.Resampling.NEAREST)
cx=x+(box[0]+box[2])/2;bottom=y+box[3];nx=round(cx-repair.width/2);ny=bottom-repair.height
im.paste((0,0,0,0),(x,y,x+w,y+h));im.alpha_composite(repair,(nx,ny));im.save(a.output)
r['repairSource']=a.repair;r['repairScale']=scale;r['destination']=[nx,ny];r['cell']=[nx,ny,repair.width,repair.height]
meta['pixelResampling']=True;meta['repairMethod']='Reviewed single-pose replacement, nearest-neighbor height match and original ground anchor'
Path(a.output+'.repack.json').write_text(json.dumps(meta,indent=2)+'\n')
