"""Non-destructive measured-crop registration. Source rectangles are never inferred grids.
One scale per sequence, foot-registration per pose. Flight lift is supplied ONLY by runtime.
"""
import json, statistics
from pathlib import Path
from PIL import Image, ImageDraw

ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/'ArtSource/Bosses/IronMatriarch/sprites-v2'
OUT=ROOT/'ArtSource/Bosses/IronMatriarch/runtime-v1'
OUT.mkdir(exist_ok=True)
manifest=json.loads((SRC/'manifest.json').read_text())
records=[]
for sheet in manifest['sheets']:
    source=Image.open(SRC/sheet['file']).convert('RGBA')
    frames=[]; boxes=[]
    for x,y,w,h in sheet['rects']:
        assert x>=0 and y>=0 and x+w<=source.width and y+h<=source.height
        frame=source.crop((x,y,x+w,y+h))
        box=frame.getchannel('A').point(lambda a:255 if a>64 else 0).getbbox()
        assert box, sheet['id']
        frames.append(frame);boxes.append(box)
    character=sheet['type']=='character'
    # Scale against the ready/recovered pose, not changing wing/flight extents.
    neutral=[0,len(frames)-1] if 'slam' in sheet['id'] else list(range(len(frames)))
    height=statistics.median(boxes[i][3]-boxes[i][1] for i in neutral)
    # Slam neutral poses have crouched knees: matching silhouette height would
    # enlarge their skulls/chest. Keep that intentional crouch at a fixed scale.
    scale=(280 if 'flying-slam' in sheet['id'] else 350)/height if character else 1
    atlas=Image.new('RGBA',(2048,512*((len(frames)+3)//4)))
    aligned=[];meta=[]
    for i,(frame,box) in enumerate(zip(frames,boxes)):
        if character:
            # Feet are the two central bottom contacts, excluding the tail.
            mask=frame.getchannel('A').point(lambda a:255 if a>64 else 0)
            footbox=mask.crop((int(frame.width*.20),int(frame.height*.65),int(frame.width*.78),frame.height)).getbbox()
            bottom=int(frame.height*.65)+footbox[3] if footbox else box[3]
            # Use the foot mass center, not the asymmetric wing/tail silhouette.
            footband=mask.crop((int(frame.width*.16),max(0,bottom-20),int(frame.width*.78),bottom)).getbbox()
            center=int(frame.width*.16)+(footband[0]+footband[2])/2 if footband else (box[0]+box[2])/2
            size=(round(frame.width*scale),round(frame.height*scale))
            image=frame.resize(size,Image.Resampling.LANCZOS)
            offset=(round(256-center*scale),round(440-bottom*scale))
            canvas=Image.new('RGBA',(512,512));canvas.alpha_composite(image,offset)
            meta.append({'rect':sheet['rects'][i],'foot':[center,bottom],'scale':scale,'offset':offset})
        else:
            # Preserve entire effect frame and register its visible emitter/ground.
            image=frame.crop(box)
            factor=min(470/image.width,400/image.height)
            image=image.resize((round(image.width*factor),round(image.height*factor)),Image.Resampling.LANCZOS)
            canvas=Image.new('RGBA',(512,512))
            offset=(16,256-image.height//2) if sheet['id'].startswith('flame-') else ((512-image.width)//2,440-image.height)
            canvas.alpha_composite(image,offset)
            meta.append({'rect':sheet['rects'][i],'scale':factor,'offset':offset})
        opaque=canvas.getchannel('A').point(lambda a:255 if a>64 else 0).getbbox()
        assert opaque and min(opaque[:2])>=2 and max(opaque[2:])<=510, (sheet['id'],i,opaque)
        atlas.alpha_composite(canvas,((i%4)*512,(i//4)*512));aligned.append(canvas)
    name='Iron_'+sheet['id'].replace('-','_')
    atlas.save(OUT/(name+'.png'))
    # Each registered pose is visible on a checkerless neutral background, labeled.
    review=Image.new('RGB',(1024,256*((len(frames)+3)//4)),(35,40,45));draw=ImageDraw.Draw(review)
    for i,im in enumerate(aligned):
        review.paste(im.resize((256,256)),((i%4)*256,(i//4)*256),im.resize((256,256)))
        draw.text(((i%4)*256+5,(i//4)*256+5),str(i+1),fill='white')
        draw.line(((i%4)*256,(i//4)*256+220,(i%4+1)*256,(i//4)*256+220),fill=(80,120,120))
    review.save(OUT/(name+'_review.jpg'))
    records.append({'name':name,'frames':len(frames),'type':sheet['type'],'registration':meta})
(OUT/'registration.json').write_text(json.dumps(records,indent=2))
# Measured mouth centers in source-frame coordinates, indexed by view and pose.
# Active sustain frames 6-9 are authored individually, not one shared socket.
mouths={
 'front': [[(47,134),(111,112),(180,99),(248,113),(305,142)],[(49,145),(116,119),(182,117),(252,123),(314,149)],[(48,166),(113,150),(181,155),(250,144),(310,167)],[(47,141),(110,121),(180,121),(249,121),(314,141)],[(48,151),(116,126),(181,123),(248,131),(316,147)],[(49,142),(114,116),(183,91),(251,123),(319,143)],[(48,149),(115,128),(183,129),(251,135),(316,153)],[(49,158),(117,120),(197,84),(265,146),(327,162)],[(48,136),(113,118),(183,116),(252,118),(312,134)],[(49,134),(113,114),(183,113),(253,112),(314,134)],[(48,137),(113,116),(183,116),(252,117),(313,135)],[(49,132),(113,113),(183,110),(252,113),(313,134)]],
 'left': [[(66,188),(111,163),(156,163),(213,170),(274,184)]]*5+ [[(56,165),(112,143),(165,145),(218,159),(276,177)],[(64,175),(109,150),(167,155),(218,162),(277,177)],[(57,126),(100,155),(148,173),(215,163),(276,184)],[(62,169),(109,153),(157,145),(217,160),(276,175)]]+ [[(66,188),(111,163),(156,163),(213,170),(274,184)]]*3,
 'right': [[(101,195),(151,164),(217,153),(267,167),(310,196)]]*5+ [[(97,181),(148,147),(207,135),(258,144),(308,169)],[(92,190),(145,151),(214,145),(257,160),(306,179)],[(88,189),(147,151),(203,128),(255,143),(306,186)],[(98,180),(151,147),(217,137),(264,157),(311,177)]]+ [[(101,195),(151,164),(217,153),(267,167),(310,196)]]*3
}
rows=[]
for view in ('front','left','right'):
    record=next(r for r in records if r['name']==f'Iron_flamethrower_{view}')
    poses=[]
    for points,reg in zip(mouths[view],record['registration']):
        poses.append('{'+','.join('FVector2D(%.3ff,%.3ff)'%(x*reg['scale']+reg['offset'][0],y*reg['scale']+reg['offset'][1]) for x,y in points)+'}')
    rows.append('{'+',\n'.join(poses)+'}')
(ROOT/'Source/TheBeardAndBlade/IronMatriarchSockets.h').write_text('#pragma once\n#include "CoreMinimal.h"\n// Generated from reviewed per-pose source mouth coordinates; do not mirror.\nnamespace IronMatriarch { inline const FVector2D Mouths[3][12][5]={\n'+',\n'.join(rows)+'\n};}\n')
print('Registered',sum(r['frames'] for r in records),'frames in',len(records),'atlases')
if (SRC.parent/'front-slam-v4/row-4.png').exists():
    import runpy
    runpy.run_path(str(ROOT/'Tools/register_iron_front_v4.py'))
elif (SRC.parent/'front-slam-v3/source.png').exists():
    import runpy
    runpy.run_path(str(ROOT/'Tools/register_iron_front_detail.py'))
