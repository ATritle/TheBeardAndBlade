"""Review measured whole-silhouette crops at their unapproved source anchors."""
from pathlib import Path
import json,argparse
from PIL import Image,ImageDraw
root=Path(__file__).resolve().parents[1]/'ArtSource/HeroFullBodyV1'
spec=json.loads((root/'production-spec.json').read_text())
if (root/'additional-spec.json').exists():spec['clips']+=json.loads((root/'additional-spec.json').read_text())['clips']
p=argparse.ArgumentParser();p.add_argument('--direction',type=int);args=p.parse_args()
review=root/'review';review.mkdir(exist_ok=True)
count=0
for clip in spec['clips']:
    for d in range(8):
        if args.direction is not None and d!=args.direction:continue
        name=f"{clip['name']}_{d}";folder=root/'crops'/name
        if not (folder/'registration.json').exists():continue
        metadata=json.loads((folder/'registration.json').read_text())
        if any('error' in f for f in metadata['frames']):continue
        w,h=metadata['size'];cw,ch=w/4,h/2
        frames=[]
        for frame in metadata['frames']:
            i=frame['index'];x0,y0,x1,y1=frame['measuredRect']
            im=Image.open(folder/f'{i:02}.png').convert('RGBA')
            canvas=Image.new('RGBA',(round(cw)+160,round(ch)+160),(20,24,23,255))
            canvas.alpha_composite(im,(round(x0-i%4*cw+80),round(y0-i//4*ch+64)))
            canvas=canvas.convert('RGB');canvas.thumbnail((420,530),Image.Resampling.LANCZOS)
            result=Image.new('RGB',(440,574),(20,24,23));result.paste(canvas,((440-canvas.width)//2,0))
            draw=ImageDraw.Draw(result);draw.text((12,540),f'{name} / pose {i+1}/8',fill=(230,210,170))
            draw.text((12,557),'Source study / anchors not approved',fill=(155,172,162))
            frames.append(result)
        if len(frames)!=8:continue
        frames[0].save(review/f'{name}.gif',save_all=True,append_images=frames[1:],duration=clip['previewFrameMs'],loop=0)
        board=Image.new('RGB',(1760,1148),(20,24,23))
        for f,im in enumerate(frames):board.paste(im,((f%4)*440,(f//4)*574))
        board.save(review/f'{name}.jpg',quality=93);count+=1
print('Measured-crop review previews:',count)
