"""Non-destructive registration review for the repaired full-body walk.

This is an inspection artifact, not an importer or an approval tool. The raw
landmarks remain estimates; in particular, occluded feet can be mislabeled.
"""
from pathlib import Path
import json
import math
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / 'ArtSource/HeroFullBodyV1'
NAME = 'Walk_SE_corrected'
OUT = ART / 'registration-review' / NAME
SIZE = 512
ROOT_POINT = (256, 440)
HEIGHT = 340


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    measurements = json.loads((ART / 'landmarks' / f'{NAME}.json').read_text())
    sources = json.loads((ART / 'crops' / NAME / 'source-records.json').read_text())
    frames, records = [], []
    board = Image.new('RGB', (4 * SIZE, 2 * SIZE), '#202522')
    for i, measurement in enumerate(measurements['frames']):
        source = ART / 'crops' / NAME / f'{i:02}.png'
        im = Image.open(source).convert('RGBA')
        scale = HEIGHT / im.height
        marks = measurement['landmarks']
        # Horizontal torso centering is for comparison only. True world roots
        # require a planted-foot pass in motion, not a bounding-box guess.
        hip_x = sum(marks[k]['xy'][0] for k in ('leftHip', 'rightHip')) / 2
        offset = (round(ROOT_POINT[0] - hip_x * scale), ROOT_POINT[1] - HEIGHT)
        resized = im.resize((round(im.width * scale), HEIGHT), Image.Resampling.LANCZOS)
        canvas = Image.new('RGBA', (SIZE, SIZE))
        canvas.alpha_composite(resized, offset)
        canvas.save(OUT / f'{i:02}.png')
        points = {k: [round(p['xy'][0]*scale+offset[0], 2),
                      round(p['xy'][1]*scale+offset[1], 2)] for k,p in marks.items()}
        grip = [round(sum(points[k][axis] for k in
                      ('rightWrist', 'rightIndex', 'rightPinky')) / 3, 2)
                for axis in (0, 1)]
        record = {'index': i, 'source': sources[i], 'scale': scale,
                  'offset': list(offset), 'previewRoot': list(ROOT_POINT),
                  'rightHandEstimate': grip, 'landmarks': points,
                  'rootApproved': False, 'handApproved': False,
                  'visualApproval': False,
                  'intendedPhase': ['right-contact', 'right-loading', 'right-passing',
                                    'right-push', 'left-contact', 'left-loading',
                                    'left-passing', 'left-push'][i]}
        records.append(record)
        bg = Image.new('RGBA', (SIZE, SIZE), '#202522')
        d = ImageDraw.Draw(bg)
        for y in range(40, SIZE, 40):
            d.line((0,y,SIZE,y), fill='#2c3430')
        for x in range(16, SIZE, 40):
            d.line((x,0,x,SIZE), fill='#2c3430')
        d.line((0,ROOT_POINT[1],SIZE,ROOT_POINT[1]), fill='#879885')
        bg.alpha_composite(canvas)
        d = ImageDraw.Draw(bg)
        d.line((250,440,262,440), fill='#f2c76d', width=2)
        d.line((256,434,256,446), fill='#f2c76d', width=2)
        d.ellipse((grip[0]-4,grip[1]-4,grip[0]+4,grip[1]+4), outline='#ffae70', width=2)
        d.text((16,16), f'{i}: {record["intendedPhase"]}', fill='white')
        d.text((16,476), 'REVIEW ONLY - anchors not certified', fill='#f2c76d')
        frames.append(bg.convert('RGB'))
        board.paste(frames[-1], ((i%4)*SIZE, (i//4)*SIZE))
    board.save(OUT / 'contact-board.jpg', quality=95)
    frames[0].save(OUT / 'walk-review.gif', save_all=True, append_images=frames[1:],
                   duration=100, loop=0, disposal=2)
    # Small-size playback reveals silhouette popping hidden by a large art board.
    small = [f.resize((256,256),Image.Resampling.LANCZOS) for f in frames]
    small[0].save(OUT / 'walk-game-scale.gif', save_all=True, append_images=small[1:],
                  duration=100, loop=0, disposal=2)
    deltas = [round(math.dist(records[i]['rightHandEstimate'],
                              records[(i+1)%8]['rightHandEstimate']),2) for i in range(8)]
    result = {'status': 'registration-review-only', 'canvas': [SIZE,SIZE],
              'normalization': '340px full silhouette height; estimated pelvis X; shared lower bound',
              'warning': 'Bounding-box baseline is NOT a certified ground anchor. No runtime changes.',
              'handFrameDistances': deltas, 'frames': records}
    (OUT / 'registration-draft.json').write_text(json.dumps(result, indent=2))
    print(json.dumps({'output': str(OUT), 'frames':len(frames), 'approved':False}))


if __name__ == '__main__':
    main()
