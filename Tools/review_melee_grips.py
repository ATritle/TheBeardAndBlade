"""Read-only art review: assemble current poses with estimated grip markers."""
from pathlib import Path
import re
from PIL import Image,ImageDraw
root=Path(__file__).resolve().parents[1]
out=root/'Saved/MeleeGripReview';out.mkdir(parents=True,exist_ok=True)
clips=['Idle','Walk','Run','MeleeSlash','MeleeBackhand','MeleeCombo','Block']
rows=re.findall(r'^\{(.*)\},?$',(root/'Source/TheBeardAndBlade/FullBodyArtMetrics.h').read_text(),re.M)
for d in range(8):
    board=Image.new('RGB',(2048,7*280),(38,42,44));draw=ImageDraw.Draw(board)
    for c,name in enumerate(clips):
        atlas=Image.open(root/f'ArtSource/HeroFullBodyV1/runtime-test/FullBody_{name}_{d}.png')
        points=re.findall(r'\{([^{}]+)\}',rows[c*8+d])
        for f in range(8):
            im=atlas.crop((f%4*256,f//4*256,f%4*256+256,f//4*256+256))
            board.paste(im,(f*256,c*280),im)
            x,y,a,*_=map(float,points[f].replace('f','').split(','))
            x+=f*256;y+=c*280
            draw.ellipse((x-3,y-3,x+3,y+3),outline=(255,70,70),width=1)
            draw.text((f*256+8,c*280+255),f'{name} D{d} F{f} angle {a:.0f}',fill='white')
    board.save(out/f'direction-{d}.png')
