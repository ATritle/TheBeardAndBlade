"""Reproducible crop/alignment checks; not a substitute for anatomy review."""
import json
import re
from pathlib import Path
from collections import defaultdict
from PIL import Image
root=Path(__file__).resolve().parents[1]
folder=root/'ArtSource/EnemyExpansion/RustbladeSquire/runtime-v2'
records=json.loads((folder/'runtime-manifest.json').read_text())
assert len(records)==296
assert len({r['name'] for r in records})==296
scales=defaultdict(set)
groups=defaultdict(list)
for r in records:
    im=Image.open(root/'Content/Art/EnemyExpansion/RustbladeSquire'/f"{r['name']}.png")
    assert im.mode=='RGBA' and im.size==(384,384),r['name']
    bbox=im.getchannel('A').point(lambda v:255 if v>64 else 0).getbbox()
    assert bbox and bbox[0]>0 and bbox[1]>0 and bbox[2]<384 and bbox[3]<384,(r['name'],bbox)
    assert abs(bbox[3]-341)<=2,(r['name'],bbox)
    assert r['root']==[192,340]
    scales[r['source']].add(r['scale'])
    _,direction,state,_=r['name'].split('_')
    assert r['directionScale']==(.90 if direction=='SW' else 1.0)
    groups[(direction,state)].append(r)
for source,values in scales.items():assert len(values)==1,(source,values)
for direction in ('N','NE','E','SE','S','SW','W','NW'):
    for state,count in [('idle',4),('walk',8),('attack',13),('hurt',4),('death',8)]:
        assert len(groups[direction,state])==count
    strike=groups[direction,'attack'][6]
    assert strike['sourceFrame']==({'N':11,'NE':10,'E':10,'SE':10,'S':10,'SW':6,'W':12,'NW':12}[direction])
assert {Path(r['source']).as_posix() for r in groups['SW','attack']}=={'ArtSource/EnemyExpansion/RustbladeSquire/runtime-v2/attack-SW-v4.png'}
assert groups['SW','attack'][0]['sourceFrame']==groups['SW','attack'][-1]['sourceFrame']==0
# Confirm corrected original SW rectangles/pivots survived extraction verbatim.
original=json.loads((root/'ArtSource/EnemyExpansion/RustbladeSquire/animation-v1/atlas-manifest.json').read_text())
entry=next(e for e in original['entries'] if e['file']=='attack-SW-right-hand-v5.png')
for r in groups['SW','attack']:
    if r['source'].endswith(entry['file']):
        assert r['crop']==entry['frames'][r['sourceFrame']]
render=float(re.search(r'RenderSize=(\d+(?:\.\d*)?)f', (root/'Source/TheBeardAndBlade/RustbladeSquire.h').read_text()).group(1))
hero_scale=float(re.search(r'void Hero\(ADungeonHero\* H,float HS=([\d.]+)f', (root/'Source/TheBeardAndBlade/DungeonActors.h').read_text()).group(1))
def body_height(path,draw_size):
    im=Image.open(path)
    b=im.getchannel('A').point(lambda v:255 if v>64 else 0).getbbox()
    return (b[3]-b[1])*draw_size/im.height
hero_heights=[body_height(p,128*hero_scale) for p in (root/'Content/Art/V2').glob('Locomotion_Walk_*_*.png')]
squire_heights=[body_height(root/'Content/Art/EnemyExpansion/RustbladeSquire'/f"{r['name']}.png",render) for r in records if r['name'].split('_')[2] in ('walk','idle')]
assert hero_heights and max(squire_heights)<min(hero_heights),(max(squire_heights),min(hero_heights))
report={'runtimeFrames':len(records),'directions':8,'states':['idle','walk','attack','hurt','death'],
        'checks':'384px RGBA, safe alpha margins, shared ground root, one scale per source, authoritative SW crops, curated strike markers',
        'result':'PASS','squireStandingPixels':[round(min(squire_heights),2),round(max(squire_heights),2)],
        'adventurerWalkingPixels':[round(min(hero_heights),2),round(max(hero_heights),2)],
        'limitation':'Geometry checks do not establish natural gait or consistent anatomy.'}
(folder/'art-validation.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report))
