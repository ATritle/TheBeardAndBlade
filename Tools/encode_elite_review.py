"""Encode Unreal-authored screenshots at their actual capture times."""
import argparse, subprocess
from pathlib import Path
from PIL import Image,ImageDraw
p=argparse.ArgumentParser();p.add_argument("ffmpeg");p.add_argument("--all",action="store_true");p.add_argument("--alignment",action="store_true");p.add_argument("--duel",action="store_true");a=p.parse_args()
root=Path(__file__).resolve().parents[1]
capture=root/("Saved/AllExpansionReplay" if a.all else "Saved/AlignmentReplay" if a.alignment else "Saved/EliteReplay")
prefix="All-enemies" if a.all else "Alignment" if a.alignment else "Elite"
movie="All_Enemies_Animation_Test.mp4" if a.all else "Alignment_Projectile_Test.mp4" if a.alignment else "Elite_Animation_Test.mp4"
if a.duel:capture=root/'Saved/DuelReplay';prefix='Templar-Duelist';movie='Templar_Duelist_HD_Test.mp4'
frames=sorted(capture.glob("frame-*.png"))
assert frames
out=root/"ArtSource/EnemyExpansion/HomePCHandoff-v2"
concat=capture/"frames.txt"
lines=[]
for i,path in enumerate(frames):
    index=int(path.stem.split("-")[-1])
    next_index=int(frames[i+1].stem.split("-")[-1]) if i+1<len(frames) else index+1
    lines += ["file '"+path.as_posix()+"'",f"duration {(next_index-index)/10:.3f}"]
lines.append("file '"+frames[-1].as_posix()+"'")
concat.write_text("\n".join(lines)+"\n")
subprocess.run([a.ffmpeg,"-y","-f","concat","-safe","0","-i",str(concat),"-r","30","-c:v","libx264","-crf","18","-pix_fmt","yuv420p","-movflags","+faststart",str(out/movie)],check=True)
for segment in range(2 if a.duel else 20 if a.all else 5 if a.alignment else 4):
    selected=[min(frames,key=lambda f:abs(int(f.stem.split("-")[-1])-(segment*140+t*10))) for t in (1,3,5,7,9,11,12,13)]
    board=Image.new("RGB",(1280,4*420),(25,28,26));d=ImageDraw.Draw(board)
    for i,path in enumerate(selected):
        im=Image.open(path).resize((640,400))
        x,y=i%2*640,i//2*420;board.paste(im,(x,y));d.text((x+8,y+402),path.stem,fill="white")
    board.save(out/f"{prefix}-gameplay-{segment+1}.jpg",quality=94)
print(out/movie)
