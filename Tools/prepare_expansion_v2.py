"""Bake every measured v2 pose into padded animation atlases and QA boards.

No uniform source slicing, duplicate-frame substitutions or discarded poses.
Calibration is per source sheet, not per action pose (so strikes do not pump).
"""
import argparse
import json
from functools import lru_cache
from pathlib import Path
from statistics import median
import numpy as np
from PIL import Image, ImageDraw
from prepare_expansion import cut, body_height, image

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "ArtSource/EnemyExpansion"
CELL, RX, RY = 384, 192, 338
DIRS = ["N","NE","E","SE","S","SW","W","NW"]

def read(p):
    return json.loads(p.read_text(encoding="utf-8-sig"))

def export(name):
    # Re-bake from the measured original poses, not an upscale of the old atlas.
    density = 2 if name in ('PermafrostTemplar','TempestDuelist') else 1
    CELL, RX, RY = 384*density, 192*density, 338*density
    src = BASE/name/"animation-v2"
    review = BASE/name/"runtime-v2"
    out = ROOT/"Content/Art/EnemyExpansion"/name
    review.mkdir(exist_ok=True)
    out.mkdir(parents=True, exist_ok=True)
    manifest, spec = read(src/"atlas-manifest.json"), read(src/"enemy-spec.json")
    overrides = read(review/"calibration.json") if (review/"calibration.json").exists() else {}
    target = (132 if spec["provisionalTier"] == "Elite" else 103)*density
    records, calibration = [], {}
    for entry in manifest["entries"]:
        state, direction = entry["state"], entry["direction"]
        frames = entry["frames"]
        assert frames
        ims = [cut(src/f.get("sourceFile",entry["file"]),f) for f in frames]
        scales = {}
        for i, f in enumerate(frames):
            file = f.get("sourceFile",entry["file"])
            if file in scales:
                continue
            indices = [j for j,g in enumerate(frames) if g.get("sourceFile",entry["file"]) == file]
            refs = indices[-3:] if "-b-" in file else indices[:3]
            if direction == "FX":
                scale = 220*density / max(max(im.size) for im in ims)
            else:
                # Anchor neutral anatomy, not weapon reach, to the shared body scale.
                scale = target / median(body_height(ims[j],frames[j]) for j in refs)
            scale *= overrides.get(file, {}).get("scaleMultiplier", 1)
            scales[file] = scale
            calibration[file] = {"scale":scale,"referencePoses":[j+1 for j in refs]}
        order = overrides.get("sequence:"+direction+":"+state)
        if order is not None:
            assert sorted(order) == list(range(len(frames))), "Every measured pose must be retained once"
            frames = [frames[i] for i in order]
            ims = [ims[i] for i in order]
        rows = (len(frames)+3)//4
        atlas = Image.new("RGBA",(CELL*4,CELL*rows))
        board = Image.new("RGB",(CELL*4,(CELL+22)*rows),(29,35,32))
        draw = ImageDraw.Draw(board)
        poses = []
        for i,(im,f) in enumerate(zip(ims,frames)):
            file = f.get("sourceFile",entry["file"])
            bb = im.getbbox()
            scale = scales[file]
            if direction == "FX":
                px,py = im.width/2,im.height/2
                rx,ry = CELL/2,CELL/2
            else:
                # Source grid pivots are not anatomical anchors on repacked sheets.
                # Use the center of the grounded footprint, excluding upper weapons.
                alpha = np.asarray(im.getchannel("A"))
                band = max(3,round((bb[3]-bb[1])*.12))
                yy,xx = np.nonzero(alpha[max(bb[1],bb[3]-band):bb[3]]>64)
                px = float(np.median(xx)) if len(xx) else (bb[0]+bb[2])/2
                py = bb[3]-1
                rx,ry = RX,RY
                if state == "death":
                    px = (bb[0]+bb[2])/2
            patch = im.resize((max(1,round(im.width*scale)),max(1,round(im.height*scale))),Image.Resampling.NEAREST)
            dx,dy = round(rx-px*scale),round(ry-py*scale)
            bounds = patch.getbbox()
            assert dx+bounds[0]>=2 and dx+bounds[2]<=CELL-2 and dy+bounds[1]>=2 and dy+bounds[3]<=CELL-2, (name,file,i,dx,dy,bounds,"needs reviewed calibration")
            canvas = Image.new("RGBA",(CELL,CELL))
            canvas.alpha_composite(patch,(dx,dy))
            ox,oy = (i%4)*CELL,(i//4)*CELL
            atlas.alpha_composite(canvas,(ox,oy))
            by = (i//4)*(CELL+22)
            board.paste(canvas,(ox,by),canvas)
            draw.line((ox,by+RY,ox+CELL,by+RY), fill=(64,90,70))
            draw.text((ox+4,by+CELL+3),f"{direction} {state} {i} / {f.get('sourcePose',i+1)}",fill="white")
            poses.append({"sourceFile":file,"sourcePose":f.get("sourcePose",i+1),"crop":f,"scale":scale,"pivot":[px,py],"destination":[dx,dy]})
        asset = f"{name}_{direction}_{state}_atlas"
        atlas.save(out/(asset+".png"))
        board.save(review/f"review-{direction}-{state}.png")
        records.append({"name":asset,"state":state,"direction":direction,"frames":len(frames),"columns":4,"rows":rows,"cell":CELL,"root":[RX,RY],"poses":poses})
    (review/"runtime-manifest.json").write_text(json.dumps(records,indent=2)+"\n")
    (review/"measured-calibration.json").write_text(json.dumps(calibration,indent=2)+"\n")
    image.cache_clear()
    print(name,sum(r["frames"] for r in records),"poses",len(records),"atlases",flush=True)

if __name__ == "__main__":
    p=argparse.ArgumentParser()
    p.add_argument("--enemy",action="append")
    args=p.parse_args()
    names=args.enemy or [e["folder"] for e in read(BASE/"HomePCHandoff-v2/inventory.json")["enemies"]]
    for name in names:
        export(name)
