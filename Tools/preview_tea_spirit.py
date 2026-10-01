"""Encode the latest engine screenshot run as an unaltered animated GIF."""
from pathlib import Path
from PIL import Image
import argparse

parser=argparse.ArgumentParser()
parser.add_argument("--capture",default="Saved/TeaSpiritReview")
parser.add_argument("--fps",type=float,default=10)
parser.add_argument("--output",default="tea-spirit-in-game.gif")
args=parser.parse_args()

root = Path(__file__).resolve().parents[1]
source = root / args.capture
paths = list(source.glob("frame-*.png"))
latest = max(p.stat().st_mtime for p in paths)
# Review runs last eight seconds. Omit stale frames skipped by the latest run.
paths = sorted(p for p in paths if p.stat().st_mtime >= latest - 15)
frames = []
durations = []
for i, path in enumerate(paths):
    with Image.open(path) as image:
        frames.append(image.convert("RGB"))
    current = int(path.stem.split("-")[-1])
    following = int(paths[i+1].stem.split("-")[-1]) if i+1 < len(paths) else current+1
    durations.append(max(round(1000/args.fps), round((following-current)*1000/args.fps)))
output = root / "Saved/TeaSpiritPreview"
output.mkdir(exist_ok=True)
target = output / args.output
palette = frames[0].quantize(colors=256)
frames = [frame.quantize(palette=palette, dither=Image.Dither.NONE) for frame in frames]
frames[0].save(target, save_all=True, append_images=frames[1:],
               duration=durations, loop=0, optimize=True, disposal=1)
print(f"{target}\n{len(frames)} frames; {sum(durations)/1000:.1f} seconds; {target.stat().st_size} bytes")
