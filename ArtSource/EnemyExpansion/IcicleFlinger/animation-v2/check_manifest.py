"""Read-only source/crop verification; does not prove artistic continuity."""
from pathlib import Path
import json
from PIL import Image
root=Path(__file__).resolve().parent
m=json.loads((root/'atlas-manifest.json').read_text())
report=[]
for e in m['sheets']:
    rows={}; overlaps=0; border=[]
    for n,f in enumerate(e['frames']):
        x,y,w,h=f['cell']
        if f.get('clipRows'):
            for ry,rx,rw in f['clipRows']:
                yy=y+ry; a=x+rx; b=a+rw
                for aa,bb,other in rows.get(yy,[]):
                    if max(a,aa)<min(b,bb): overlaps+=1
                rows.setdefault(yy,[]).append((a,b,n))
                if yy in (0,e['height']-1) or a==0 or b==e['width']:
                    border.append(n+1)
    im=Image.open(root/e['file']).convert('RGBA')
    alpha=im.getchannel('A')
    report.append(dict(file=e['file'],size=im.size,alpha=alpha.getextrema(),
        poses=len(e['frames']),components=e['componentCount'],empty=e['emptyCells'],
        overlappingMaskRows=overlaps,sourceBorderPoses=sorted(set(border)),
        cropWarnings=e['edgeRiskCells']))
result={'sourceCheckOnly':True,'sheets':len(report),'checks':report,
    'walkCells':sum(len(e['frames']) for e in m['entries'] if e['state']=='walk'),
    'attackCells':sum(len(e['frames']) for e in m['entries'] if e['state']=='attack')}
(root/'crop-check.json').write_text(json.dumps(result,indent=2))
print(json.dumps(result))
