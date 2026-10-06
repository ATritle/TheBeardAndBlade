"""Encode unaltered engine screenshots at their captured simulation times."""
from pathlib import Path
from PIL import Image,ImageDraw
root=Path(__file__).resolve().parents[1]
folder=root/'Saved/Screenshots/GroundedReplay'
paths=sorted(folder.glob('Frame*.png'))
latest=max(p.stat().st_mtime for p in paths)
paths=[p for p in paths if p.stat().st_mtime>latest-45]
out=root/'ArtSource/HeroGroundedV3'
palette=Image.open(paths[len(paths)//2]).convert('RGB').quantize(colors=256)
frames=[];durations=[]
for i,p in enumerate(paths):
    frames.append(Image.open(p).convert('RGB').quantize(palette=palette,dither=Image.Dither.NONE))
    current=int(p.stem[5:]);following=int(paths[i+1].stem[5:]) if i+1<len(paths) else current+1
    durations.append(round((following-current)/24*1000))
frames[0].save(out/'Gameplay.gif',save_all=True,append_images=frames[1:],duration=durations,loop=0,optimize=False)
board=Image.new('RGB',(4*384,3*290),(20,25,30));draw=ImageDraw.Draw(board)
for i,t in enumerate((.2,.8,1.4,2,2.6,3.2,4.6,5.3,6.9,7.8,9,10.3)):
    p=min(paths,key=lambda p:abs(int(p.stem[5:])/24-t));im=Image.open(p)
    # Full frame, no synthetic character rendering in this engine review.
    im.thumbnail((384,256));x=i%4*384;y=i//4*290;board.paste(im,(x,y));draw.text((x+4,y+260),f'{p.stem} / {t}s',fill='white')
board.save(out/'Gameplay-review.jpg',quality=95)
print(f'{len(paths)} frames / {sum(durations)/1000:.2f}s / {out / "Gameplay.gif"}')
