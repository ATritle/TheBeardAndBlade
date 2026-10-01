"""Encode engine-captured HUD review frames using their simulation timestamps."""
import argparse
import subprocess
from pathlib import Path

parser=argparse.ArgumentParser()
parser.add_argument("ffmpeg")
parser.add_argument("--capture", default="Saved/HotbarReviewPolished")
parser.add_argument("--output", default="ArtSource/UI/Hotbar/v3/HUD_Block_Review.mp4")
args=parser.parse_args()
root=Path(__file__).resolve().parents[1]
capture=root/args.capture
frames=sorted(capture.glob("frame-*.png"))
assert frames, "Run -HotbarReview first"
lines=[]
for index,path in enumerate(frames):
    number=int(path.stem.split("-")[-1])
    following=int(frames[index+1].stem.split("-")[-1]) if index+1<len(frames) else number+1
    lines.extend(["file '"+path.as_posix()+"'",f"duration {(following-number)/10:.3f}"])
lines.append("file '"+frames[-1].as_posix()+"'")
manifest=capture/"frames.txt"
manifest.write_text("\n".join(lines)+"\n",encoding="utf-8")
output=root/args.output
subprocess.run([args.ffmpeg,"-y","-f","concat","-safe","0","-i",str(manifest),"-r","30","-c:v","libx264","-crf","18","-pix_fmt","yuv420p","-movflags","+faststart",str(output)],check=True)
print(output)
