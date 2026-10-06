from pathlib import Path
from PIL import Image, ImageDraw
root=Path(__file__).resolve().parents[1]
out=root/'ArtSource/HeroGroundedV3';out.mkdir(exist_ok=True)
board=Image.new('RGB',(4*384,2*420),(35,40,44));d=ImageDraw.Draw(board)
for direction in range(8):
    im=Image.open(root/f'Content/Art/V2/Locomotion_Walk_{direction}_0.png').convert('RGBA')
    x,y=direction%4*384,direction//4*420
    board.paste(im,(x,y),im)
    for v in range(40,129,8):
        d.line((x,y+v*3,x+384,y+v*3),fill=(65,70,75));d.text((x,y+v*3),str(v),fill='white')
    for u in range(16,129,16):
        d.line((x+u*3,y,x+u*3,y+384),fill=(65,70,75));d.text((x+u*3,y+390),str(u),fill='white')
    d.text((x+8,y+405),f'D{direction}',fill='yellow')
board.save(out/'RigSourceReview.png')
