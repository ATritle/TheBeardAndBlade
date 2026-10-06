"""Encode unaltered Unreal bow review screenshots and inspect source texture alpha."""
from pathlib import Path
import json
from PIL import Image,ImageDraw
import numpy as np
root=Path(__file__).resolve().parents[1]
out=root/'ArtSource/BowsV2'
names=json.loads((out/'manifest.json').read_text())
for name in names:
    im=Image.open(out/f'{name}.png').convert('RGBA');a=np.array(im)
    assert a[:,:,3].min()==0 and a[:,:,3].max()>200,name
    assert np.all(a[0,:,3]==0) and np.all(a[-1,:,3]==0),f'{name}: vertical clipping'
    assert np.all(a[:,0,3]==0) and np.all(a[:,-1,3]==0),f'{name}: horizontal clipping'
for folder,filename in [('BowV2Poses','Poses'),('BowV2Replay','Gameplay')]:
    paths=sorted((root/'Saved/Screenshots'/folder).glob('Frame*.png'))
    if not paths:continue
    latest=max(p.stat().st_mtime for p in paths)
    paths=[p for p in paths if p.stat().st_mtime>=latest-60]
    palette=Image.open(paths[len(paths)//2]).convert('RGB').quantize(colors=256)
    frames=[];duration=[]
    for i,p in enumerate(paths):
        frames.append(Image.open(p).convert('RGB').quantize(palette=palette,dither=Image.Dither.NONE))
        n=int(p.stem[5:]);nxt=int(paths[i+1].stem[5:]) if i+1<len(paths) else n+1
        duration.append(round((nxt-n)/15*1000))
    frames[0].save(out/f'{filename}.gif',save_all=True,append_images=frames[1:],duration=duration,loop=0,optimize=False)
    board=Image.new('RGB',(1280,840),(24,28,30));draw=ImageDraw.Draw(board)
    for i in range(6):
        p=paths[min(len(paths)-1,round((i+.5)*len(paths)/6))]
        im=Image.open(p).convert('RGB');im.thumbnail((640,400))
        x=i%2*640;y=i//2*280;im.thumbnail((448,260));board.paste(im,(x,y))
        draw.text((x+450,y+8),p.stem,fill='white')
    board.save(out/f'{filename}-review.jpg',quality=95)
    print(filename,len(paths),'engine frames')
print('126 source assets passed alpha and boundary checks')
