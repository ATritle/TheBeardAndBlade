"""Extract authored turntable poses; no painting, mirroring or synthesized tween frames."""
import json
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw
from prepare_rustblade import state_cells

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'ArtSource/InventoryExpansion/Turntable-v1'
OUT = ROOT / 'Content/Art/V2'
records, frames = [], []
for q, filename in enumerate(['quarter-0.png', 'quarter-1.png', 'quarter-2-v2.png', 'quarter-3-v2.png']):
    sheet = Image.open(SOURCE / filename).convert('RGBA')
    for n, meta in enumerate(state_cells(sheet, rows=2)):
        x, y, w, h = meta['cell']
        crop = sheet.crop((x, y, x+w, y+h)).crop(tuple(meta['bounds']))
        if q == 3 and n == 7:
            seam = Image.open(SOURCE/'seam-v1.png').convert('RGBA')
            crop = seam.crop((seam.width//2,0,seam.width,seam.height))
            bounds = Image.fromarray(np.asarray(crop)[:,:,3]>64).getbbox()
            crop = crop.crop(bounds)
            meta = dict(source='seam-v1.png', panel=1, bounds=list(bounds))
        a = np.array(crop.getchannel('A')); a[a < 32] = 0
        crop.putalpha(Image.fromarray(a))
        # Bald crown anchors the body axis, rather than the cape's variable width.
        crown = np.nonzero(a[:max(1, round(crop.height*.06))] > 128)[1]
        axis = float(np.median(crown))
        scale = 704 / crop.height
        resized = crop.resize((round(crop.width*scale), 704), Image.Resampling.LANCZOS)
        left = round(256-axis*scale)
        assert 0 <= left and left+resized.width <= 512, (q,n,left,resized.size)
        frame = Image.new('RGBA', (512,768)); frame.alpha_composite(resized,(left,32))
        name = f'InventoryTurn_{q*8+n:02d}'
        frame.save(OUT / f'{name}.png')
        records.append(dict(name=name, source=filename, crop=meta, axis=axis, scale=scale))
        frames.append(frame)
(SOURCE/'manifest.json').write_text(json.dumps(records,indent=2)+'\n')
contact=Image.new('RGB',(1024,832),(21,25,23)); draw=ImageDraw.Draw(contact)
for i,frame in enumerate(frames):
    small=frame.resize((128,192),Image.Resampling.LANCZOS)
    contact.paste(small,((i%8)*128,(i//8)*208),small)
    draw.text(((i%8)*128+5,(i//8)*208+192),str(i),fill='white')
contact.save(SOURCE/'contact-sheet.jpg')
preview=[]
for frame in frames:
    bg=Image.new('RGB',(384,576),(21,25,23)); fg=frame.resize(bg.size,Image.Resampling.LANCZOS)
    bg.paste(fg,(0,0),fg);preview.append(bg)
preview[0].save(SOURCE/'rotation-preview.gif',save_all=True,append_images=preview[1:],duration=110,loop=0)
print(f'Prepared {len(records)} inventory-only frames')
