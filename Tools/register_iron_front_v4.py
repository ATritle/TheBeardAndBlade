"""Measured front-only pose registration; keep approved sequence silhouette sizes."""
import json
from pathlib import Path
from PIL import Image, ImageDraw
ROOT=Path(__file__).resolve().parents[1]
ART=ROOT/'ArtSource/Bosses/IronMatriarch'
SOURCE=ART/'front-slam-v4'
regions=[
 [(0,0,617,601),(620,0,1254,603),(0,815,620,1220),(622,600,1254,1220)],
 [(0,0,600,620),(600,0,1254,620),(0,690,685,1230),(685,690,1254,1230)],
 [(35,150,595,620),(655,0,1160,620),(100,625,560,1210),(570,650,1254,1200)],
 [(0,400,625,710),(640,290,1254,710),(0,735,620,1200),(625,705,1254,1200)]]
# Stable target bounds from the original measured registration, not the current
# atlas. Re-running this script must not accumulate registration/resampling drift.
targets=json.loads((ART/'front-slam-v3/registration.json').read_text())
atlas=Image.new('RGBA',(2048,2048))
review=Image.new('RGB',(1024,1024),(36,40,44))
records=[]
for row,rectangles in enumerate(regions):
    for col,rect in enumerate(rectangles):
        i=row*4+col
        # Match wing phase, not simply generated reading order: the second
        # group's first image is already the raised-wing pose (original 6).
        source_row=row
        if i==4:source_row=0;rect=regions[0][3]
        elif 5<=i<=7:rect=regions[1][i-5]
        source=Image.open(SOURCE/f'row-{source_row+1}.png').convert('RGBA')
        cut=source.crop(rect)
        bbox=cut.getchannel('A').point(lambda a:255 if a>64 else 0).getbbox()
        assert bbox and bbox[0]>0 and bbox[1]>0 and bbox[2]<cut.width and bbox[3]<cut.height,(i+1,bbox,rect)
        cut=cut.crop(bbox)
        # User-approved edge cleanup: remove faint matte/colored fringe while
        # preserving opaque RGB. A binary silhouette suits the pixel-art sampler.
        cut.putalpha(cut.getchannel('A').point(lambda a:255 if a>=160 else 0))
        target=targets[i]['target_bounds']
        cut=cut.resize((target[2]-target[0],target[3]-target[1]),Image.Resampling.LANCZOS)
        cut.putalpha(cut.getchannel('A').point(lambda a:255 if a>=128 else 0))
        canvas=Image.new('RGBA',(512,512));canvas.alpha_composite(cut,target[:2])
        atlas.alpha_composite(canvas,(col*512,row*512))
        thumb=canvas.resize((256,256),Image.Resampling.NEAREST)
        review.paste(thumb,(col*256,row*256),thumb)
        records.append({'pose':i+1,'source':f'row-{source_row+1}.png','crop':[rect[0]+bbox[0],rect[1]+bbox[1],bbox[2]-bbox[0],bbox[3]-bbox[1]],'target_bounds':target})
atlas.save(ART/'runtime-v1/Iron_flying_slam_front.png')
alpha_hist=atlas.getchannel('A').histogram()
assert sum(alpha_hist[1:255])==0, 'No translucent halo pixels may survive'
review.save(SOURCE/'registered-review.png')
(SOURCE/'registration.json').write_text(json.dumps(records,indent=2))
print('Registered all 16 front-only frames; source crops contained; original target bounds retained')
