"""Non-destructive source QA and previews. Never imports unapproved art into UE.

Eight-cell layouts are proposals, not a guarantee. Alpha bounds and cell edges
are checked and exposed for manual review; failed cells are not silently fixed.
No body-part separation, pose warping, or interpolated duplicate frames.
"""
from pathlib import Path
import json, argparse, hashlib
import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT/'ArtSource/HeroFullBodyV1'
SPEC = json.loads((OUT/'production-spec.json').read_text())
if (OUT/'additional-spec.json').exists():SPEC['clips']+=json.loads((OUT/'additional-spec.json').read_text())['clips']
SELECTIONS = json.loads((OUT/'selections.json').read_text()) if (OUT/'selections.json').exists() else {}
parser = argparse.ArgumentParser()
parser.add_argument('--direction', type=int)
args = parser.parse_args()
review = OUT/'review'
review.mkdir(exist_ok=True)
records = []
for clip in SPEC['clips']:
    for d in range(8):
        name = f"{clip['name']}_{d}"
        path = OUT/SELECTIONS.get(name, f'production/{name}.png')
        record = {'name':name, 'status':'missing', 'visualApproval':False,
                  'anchorsApproved':False, 'engineVerified':False}
        records.append(record)
        if not path.exists():
            continue
        sheet = Image.open(path).convert('RGBA')
        w,h = sheet.size
        record.update(status='source-needs-review', size=[w,h], frames=[])
        frames = []
        signatures = []
        for f in range(8):
            x0,x1 = round(f%4*w/4), round((f%4+1)*w/4)
            y0,y1 = round(f//4*h/2), round((f//4+1)*h/2)
            im = sheet.crop((x0,y0,x1,y1))
            a = np.asarray(im)[:,:,3]
            ys,xs = np.where(a>32)
            if not len(xs):
                record['frames'].append({'index':f,'error':'empty alpha'})
                continue
            bbox = [int(xs.min()),int(ys.min()),int(xs.max()+1),int(ys.max()+1)]
            border = bool((a[0,:]>32).any() or (a[-1,:]>32).any() or
                          (a[:,0]>32).any() or (a[:,-1]>32).any())
            record['frames'].append({'index':f,'sourceCell':[x0,y0,x1,y1],
                                    'alphaBounds':bbox,'touchesCellEdge':border,
                                    'opaqueFraction':round(float((a>32).mean()),4)})
            signatures.append(hashlib.sha256(im.tobytes()).hexdigest())
            bg = Image.new('RGBA',im.size,(20,24,23,255))
            composed = Image.alpha_composite(bg,im).convert('RGB')
            composed.thumbnail((384,512),Image.Resampling.LANCZOS)
            canvas = Image.new('RGB',(400,550),(20,24,23))
            canvas.paste(composed,((400-composed.width)//2,12))
            draw = ImageDraw.Draw(canvas)
            draw.text((12,520),f'{name} / {f+1}/8 / SOURCE REVIEW',fill=(230,210,170))
            frames.append(canvas)
        record['exactDuplicateFrames'] = 8-len(set(signatures))
        record['technicalFlags'] = [f"frame {f['index']}: {f.get('error','cell-edge contact')}"
                                    for f in record['frames'] if f.get('error') or f.get('touchesCellEdge')]
        if len(frames)==8 and (args.direction is None or args.direction==d):
            frames[0].save(review/f'{name}.gif',save_all=True,append_images=frames[1:],
                           duration=clip['previewFrameMs'],loop=0)
            board = Image.new('RGB',(1600,1100),(20,24,23))
            for f,im in enumerate(frames): board.paste(im,((f%4)*400,(f//4)*550))
            board.save(review/f'{name}.jpg',quality=93)
report = {'requiredSheets':len(records),'presentSheets':sum(r['status']!='missing' for r in records),
          'approvedSheets':0,'complete':False,'records':records,
          'note':'Pixel tests do not certify anatomy, gait, directional accuracy or seamless playback. Manual visual and anchor review required.'}
(OUT/'qa-report.json').write_text(json.dumps(report,indent=2))
print(json.dumps({k:report[k] for k in ['requiredSheets','presentSheets','approvedSheets','complete']}))
