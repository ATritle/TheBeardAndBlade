"""Generate alpha-composited, gameplay-sized sequence contact boards for review."""
import argparse,json
from pathlib import Path
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'ArtSource/EnemyExpansion'
p=argparse.ArgumentParser();p.add_argument('--enemy',action='append');p.add_argument('--timing',action='store_true');a=p.parse_args()
names=a.enemy or [e['folder'] for e in json.loads((BASE/'HomePCHandoff-v2/inventory.json').read_text())['enemies']]
for name in names:
    folder=BASE/name/'runtime-v2'
    records=json.loads((folder/'runtime-manifest.json').read_text())
    for state in (['attack','ranged'] if a.timing else ['idle','walk','hurt','death']):
        entries=[r for r in records if r['state']==state and r['direction']!='FX']
        if not entries:continue
        indices=list(range(6,12)) if a.timing else [0,1,2,3,4,5]
        if not a.timing:
            # Six distributed samples include the endpoint, with the exact frame printed.
            indices=sorted(set(round(i*(entries[0]['frames']-1)/5) for i in range(6)))
        board=Image.new('RGB',(6*200,8*190),(29,35,32));draw=ImageDraw.Draw(board)
        for row,r in enumerate(entries):
            atlas=Image.open(ROOT/'Content/Art/EnemyExpansion'/name/(r['name']+'.png'))
            for col,f in enumerate(indices):
                x,y=f%4*384,f//4*384
                im=atlas.crop((x+92,y+170,x+292,y+350))
                board.paste(im,(col*200,row*190),im)
                draw.text((col*200+3,row*190+177),f"{r['direction']} {state} {f}",fill='white')
        board.save(folder/('compact-'+state+'.png'))
    print(name,flush=True)
