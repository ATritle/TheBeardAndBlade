"""Lossless measured crops of approved TEA source; no per-frame auto-fitting."""
import json
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/'ArtSource/Tea/effects-v4'
OUT=ROOT/'ArtSource/Tea/runtime-v4';OUT.mkdir(exist_ok=True)
pack=json.loads((SRC/'manifest.json').read_text())
names=['Flight','Ground','Enemy']
# Gameplay proportions, not the oversized source-preview presentation.
# Roughly 25 world units of visible cup; impact cup remains comparable.
widths=[38,112,124]
anchors=[
 [(222,265)]*8,
 [(246,402),(252,405),(254,405),(270,315),(242,340),(246,350)],
 [(355,295),(302,286),(379,292),(315,297)]]
lines=['#pragma once','#include "CoreMinimal.h"','namespace TeaV4 {','struct FMetric { float X,Y,W,H; };']
records=[]
for k,s in enumerate(pack['sheets']):
    im=Image.open(SRC/s['file']).convert('RGBA')
    scale=widths[k]/max(s['cellSize'])
    values=[]
    assert len(s['rects'])==len(anchors[k])
    for i,(x,y,w,h) in enumerate(s['rects']):
        assert x>=0 and y>=0 and x+w<=im.width and y+h<=im.height
        frame=im.crop((x,y,x+w,y+h))
        assert frame.getchannel('A').getbbox()
        name=f'TeaV4_{names[k]}_{i}'
        frame.save(OUT/(name+'.png'))
        ax,ay=anchors[k][i]
        values.append('{'+','.join(f'{v:.5f}f' for v in [ax*scale,ay*scale,w*scale,h*scale])+'}')
        records.append({'name':name,'rect':[x,y,w,h],'anchor':[ax,ay],'scale':scale})
    lines.append(f'inline const FMetric {names[k]}[]={{'+','.join(values)+'};')
lines.append(f'inline const FVector2D HandGrip({365*widths[0]/444:.5f}f,{270*widths[0]/444:.5f}f);')
lines.append('}')
(ROOT/'Source/TheBeardAndBlade/TeaV4Metrics.h').write_text('\n'.join(lines)+'\n')
(OUT/'registration.json').write_text(json.dumps(records,indent=2))
print('TEA: 18 lossless measured crops, sequence scales and contact anchors exported')
