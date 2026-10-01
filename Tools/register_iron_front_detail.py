"""Register the detailed front-slam edit to the approved original animation bounds.

Measured source search rectangles, NOT a uniform slicing assumption. Final tight
alpha rectangles and old/new transforms are recorded for review/reproducibility.
The original sequence's varying silhouette sizes are retained, not normalized
independently to a single height. Original source sheets remain untouched.
"""
import json
from pathlib import Path
from PIL import Image, ImageDraw

ROOT=Path(__file__).resolve().parents[1]
ART=ROOT/'ArtSource/Bosses/IronMatriarch'
source=Image.open(ART/'front-slam-v3/source.png').convert('RGBA')
regions=[
 (4,45,300,306),(326,60,613,307),(632,126,940,307),(956,45,1254,309),
 (5,375,307,614),(344,346,619,617),(621,385,943,621),(945,394,1254,622),
 (18,677,307,922),(365,641,578,926),(680,635,901,926),(961,653,1254,929),
 (6,1023,310,1203),(333,956,618,1205),(628,1001,940,1205),(956,947,1254,1207)]

atlas_path=ART/'runtime-v1/Iron_flying_slam_front.png'
original=Image.open(atlas_path).convert('RGBA')
output=Image.new('RGBA',original.size)
review=Image.new('RGB',(1024,1024),(35,40,45))
draw=ImageDraw.Draw(review)
records=[]
for i,region in enumerate(regions):
    frame=source.crop(region)
    mask=frame.getchannel('A').point(lambda a:255 if a>64 else 0)
    bbox=mask.getbbox()
    assert bbox, i
    # Verify full silhouette has transparent space on all four search edges.
    assert bbox[0]>0 and bbox[1]>0 and bbox[2]<frame.width and bbox[3]<frame.height,(i,bbox,region)
    old=original.crop(((i%4)*512,(i//4)*512,(i%4+1)*512,(i//4+1)*512))
    target=old.getchannel('A').point(lambda a:255 if a>64 else 0).getbbox()
    cut=frame.crop(bbox)
    cut=cut.resize((target[2]-target[0],target[3]-target[1]),Image.Resampling.LANCZOS)
    canvas=Image.new('RGBA',(512,512));canvas.alpha_composite(cut,target[:2])
    output.alpha_composite(canvas,((i%4)*512,(i//4)*512))
    small=canvas.resize((256,256))
    review.paste(small,((i%4)*256,(i//4)*256),small)
    draw.text(((i%4)*256+5,(i//4)*256+5),str(i+1),fill='white')
    records.append({'pose':i+1,'source_rect':[region[0]+bbox[0],region[1]+bbox[1],bbox[2]-bbox[0],bbox[3]-bbox[1]],'target_bounds':target})
output.save(atlas_path)
review.save(ART/'runtime-v1/Iron_flying_slam_front_review.jpg')
(ART/'front-slam-v3/registration.json').write_text(json.dumps(records,indent=2))
print('Registered 16 detailed front slam poses with original animation silhouettes')
