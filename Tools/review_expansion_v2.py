"""Compact cross-state boards plus reproducible atlas metadata validation."""
import json
from pathlib import Path
from PIL import Image, ImageDraw
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/"ArtSource/EnemyExpansion"
enemies=json.loads((BASE/"HomePCHandoff-v2/inventory.json").read_text())["enemies"]
reports=[]
for enemy in enemies:
    name=enemy["folder"];folder=BASE/name/"runtime-v2"
    path=folder/"runtime-manifest.json"
    if not path.exists():continue
    records=json.loads(path.read_text())
    states=["idle","walk","attack","ranged","hurt","death"]
    board=Image.new("RGB",(8*192,6*216),(30,35,32));draw=ImageDraw.Draw(board)
    for record in records:
        atlas=Image.open(ROOT/"Content/Art/EnemyExpansion"/name/(record["name"]+".png"))
        cell=record["cell"]
        assert atlas.size==(4*cell,record["rows"]*cell)
        for frame in range(record["frames"]):
            x,y=frame%4*cell,frame//4*cell
            bb=atlas.crop((x,y,x+cell,y+cell)).getbbox()
            assert bb and min(bb[:2])>=2 and max(bb[2:])<=cell-2,(name,record["name"],frame,bb)
        if record["direction"]=="FX":continue
        col=["N","NE","E","SE","S","SW","W","NW"].index(record["direction"])
        row=states.index(record["state"])
        frame=0
        im=atlas.crop((0,0,cell,cell)).resize((192,192),Image.Resampling.NEAREST)
        x,y=col*192,row*216
        board.paste(im,(x,y),im)
        draw.text((x+4,y+194),record["direction"]+" "+record["state"],fill="white")
    board.save(folder/"cross-state-overview.png")
    reports.append({"enemy":name,"atlases":len(records),"poses":sum(r["frames"] for r in records),"boundsPassed":True,"visualApproval":False})
(BASE/"HomePCHandoff-v2/runtime-atlas-audit.json").write_text(json.dumps(reports,indent=2)+"\n")
print(len(reports),"enemies",sum(r["poses"] for r in reports),"baked poses validated")
