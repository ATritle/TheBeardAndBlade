"""Preserve visible pixels; clear invisible RGB and build an alpha-composited review."""
from pathlib import Path
import json
import numpy as np
from PIL import Image, ImageDraw
root=Path(__file__).resolve().parents[1]/'ArtSource/EnemyExpansion/ThornweaveSentinel'
out=root/'continuation-v4';out.mkdir(exist_ok=True)
review=Image.new('RGB',(1200,1800),(56,66,76));draw=ImageDraw.Draw(review)
records=[]
for i,direction in enumerate(['NE','SE','S','SW','W','NW']):
    name=f'hurt-{direction}.png'
    source=root/'continuation-v3/rejected'/name
    if not source.exists():source=root/'continuation-v2/rejected'/name
    im=Image.open(source).convert('RGBA');data=np.array(im)
    data[data[:,:,3]==0,:3]=0
    dest=out/name;Image.fromarray(data).save(dest)
    bg=Image.new('RGBA',im.size,(56,66,76,255));bg.alpha_composite(Image.fromarray(data));bg.thumbnail((590,570))
    x=(i%2)*600;y=(i//2)*600
    review.paste(bg.convert('RGB'),(x,y+25));draw.text((x+10,y+5),name,fill='white')
    records.append({'source':str(source),'output':str(dest),'operation':'Set RGB=0 ONLY where alpha=0; visible pixels unchanged','status':'Awaiting visual and crop review'})
review.save(out/'hurt-alpha-review.jpg')
(out/'hurt-alpha-repairs.json').write_text(json.dumps(records,indent=2)+'\n',encoding='utf-8')
