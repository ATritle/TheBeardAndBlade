"""Mechanical crop previews using measured masks, never uniform sprite slicing."""
import json
from pathlib import Path
from PIL import Image, ImageDraw
from prepare_expansion import cut

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'ArtSource/EnemyExpansion'
for enemy,state,direction in [('RivetGunner','attack','E'),('RivetGunner','attack','NE'),('TrenchShivver','attack','E'),('TrenchShivver','attack','SW'),('BreachHound','idle','E')]:
    src=BASE/enemy/'animation-v2'
    data=json.loads((src/'atlas-manifest.json').read_text())
    entry=next(e for e in data['entries'] if e['state']==state and e['direction']==direction)
    frames=entry['frames']
    crops=[cut(src/f['sourceFile'],f) for f in frames]
    # A single scale across the sequence exposes source A/B size mismatches.
    # Bottom registration is only a review aid, not an approved runtime pivot.
    scale=112/max(im.getbbox()[3]-im.getbbox()[1] for im in crops)
    tiles=[]
    for i,(im,f) in enumerate(zip(crops,frames)):
        bb=im.getbbox(); px=f.get('pivotX',im.width/2)
        resized=im.resize((round(im.width*scale),round(im.height*scale)),Image.Resampling.NEAREST)
        tile=Image.new('RGBA',(200,160),(25,30,31,255))
        tile.alpha_composite(resized,(round(100-px*scale),round(137-bb[3]*scale)))
        ImageDraw.Draw(tile).text((8,5),f'{i+1}: {f["sourcePose"]}',fill='white')
        tiles.append(tile.convert('RGB'))
    out=BASE/enemy/'continuation-v1';out.mkdir(exist_ok=True)
    sheet=Image.new('RGB',(800,160*((len(tiles)+3)//4)))
    for i,tile in enumerate(tiles):sheet.paste(tile,((i%4)*200,(i//4)*160))
    sheet.save(out/f'{state}-{direction}-contact.png')
    tiles[0].save(out/f'{state}-{direction}-review.gif',save_all=True,append_images=tiles[1:],duration=90,loop=0)
    print(enemy,state,direction,len(tiles),'frames; source review only')
